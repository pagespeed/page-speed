/**
 * Copyright 2007-2009 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * @fileoverview Rule to find static content served from a
 * domain that sets cookies.
 *
 * @author Bryan McQuade
 */

(function() {  // Begin closure

// minimum number of resources before we warn. if there are only a few
// resources w/ cookies, it's not worth the cost of adding an extra
// domain.
var MIN_RESOURCES = 5;

/**
 * Rule to find static content served from a domain that sets cookies.
 * @this PAGESPEED.LintRule
 */
var staticNoCookieRule = function() {
  var badUrls = [];

  // Find all "static" resources. Exclude JS since we suggest serving
  // some JS on the main domain (see dnsLint.js), which may have
  // cookies.
  var urls = PAGESPEED.Utils.filterProtocols(
    PAGESPEED.Utils.getResources('css', 'image', 'object', 'cssimage'));

  var violations = 0;
  var staticComponentCount = 0;
  var cookieBytes = 0;
  for (var i = 0, len = urls.length; i < len; i++) {
    var responseCode = PAGESPEED.Utils.getResponseCode(urls[i]);
    if (!PAGESPEED.Utils.isCacheableResponseCode(responseCode)) continue;

    // Open question:  Should resources whose URL has query string parameters
    // be counted as static?  It is possible to generate custom content based
    // on the query parameters, but many sites serve static content this way
    // as well.  For now we do not test for the query parameter, and assume
    // that all objects of the types listed above are static.  Suggestions
    // for improving detection of non-static content should be sent to the
    // pagespeed mailing list.

    staticComponentCount++;

    var headers = PAGESPEED.Utils.getRequestHeaders(urls[i]);
    var cookie = headers['Cookie'];
    if (!cookie) {
      cookie = PAGESPEED.Utils.getCookieString(urls[i]);
    }
    if (!cookie) {
      continue;
    }

    violations++;
    cookieBytes += cookie.length;

    badUrls.push(urls[i]);
  }

  if (badUrls.length < MIN_RESOURCES) {
    this.score = 'n/a';
    return;
  }

  // Penalize for number of bytes incurred.
  var badPoints = (cookieBytes / 75);

  // Give them some credit for the percentage of static components
  // that did not get served from a domain that sets cookies.
  var violationPercentage = (violations / staticComponentCount);

  // Don't give too much credit based on the percentage. Give at
  // best 60% of the original penalty.
  violationPercentage = Math.max(violationPercentage, 0.6);

  badPoints *= violationPercentage;

  this.score -= badPoints;

  if (this.score < 0) {
    this.score = 0;
  }

  this.warnings =
      ['Serve the following static resources from a ',
       'domain that doesn\'t set cookies:',
       PAGESPEED.Utils.formatWarnings(badUrls),
       PAGESPEED.Utils.formatBytes(cookieBytes),
       ' of cookies were sent with the requests for these resources.'
       ].join('');
};

PAGESPEED.LintRules.registerLintRule(
  new PAGESPEED.LintRule(
    'Serve static content from a cookieless domain',
    PAGESPEED.REQUEST_GROUP,
    'request.html#ServeFromCookielessDomain',
    staticNoCookieRule,
    1.5,
    'NoCookie'
  )
);

})();  // End closure
