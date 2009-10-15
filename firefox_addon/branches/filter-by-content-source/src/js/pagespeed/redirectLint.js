/**
 * Copyright 2008-2009 Google Inc.
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
 * @fileoverview A lint rule for avoiding redirects.
 *
 * @author Tony Gentilcore
 */

(function() {  // Begin closure

/**
 * @this PAGESPEED.LintRule
 * @param {PAGESPEED.ResourceAccessor} resourceAccessor An object that
 *     allows rules to fetch content by type.
 */
var redirectRule = function(resourceAccessor) {
  // Loop through all resources and deduct 6 points per redirect.
  var redirects = PAGESPEED.Utils.getComponents().redirect;
  var aWarnings = [];
  var totalRedirects = 0;

  for (var url in redirects) {
    // Chained redirects are listed in the elements array.
    var redirectElements = redirects[url].elements;

    for (var i = 0, len = redirectElements.length; i < len; ++i) {

      // The first url in the chain is |url|, so use it when i==0.
      var fromUrl = (i == 0) ? url : redirects[url].elements[i-1];
      var toUrl = redirects[url].elements[i];

      // If a.com redirects to b.net, removing the redirect means altering
      // a.com.  Therefore, filtering should test the url redirected from.
      if (!resourceAccessor.isResourceUnderTest(url, ['redirect']))
        continue;

      aWarnings.push('From ' + fromUrl + ' to ' + toUrl);
      totalRedirects++;
      this.score -= 6;
    }
  }

  if (aWarnings.length) {
    this.warnings = ('Remove the following redirects if possible: ' +
                     PAGESPEED.Utils.formatWarnings(aWarnings));
  }

  this.addStatistics_({
    numRedirects: totalRedirects
  });
};

PAGESPEED.LintRules.registerLintRule(
  new PAGESPEED.LintRule(
    'Minimize redirects',
    PAGESPEED.RTT_GROUP,
    'rtt.html#AvoidRedirects',
    redirectRule,
    1.5,
    'MinRedirect'
  )
);

})();  // End closure
