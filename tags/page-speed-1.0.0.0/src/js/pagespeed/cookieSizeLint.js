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
 * @fileoverview Rule to find requests with large cookies.
 *
 * @author Bryan McQuade
 */

(function() {  // Begin closure

var MIN_COOKIE_SIZE_THRESHOLD_ = 400;
var MAX_COOKIE_SIZE_THRESHOLD_ = 1000;
var BYTES_PER_POINT_ =
    (MAX_COOKIE_SIZE_THRESHOLD_ - MIN_COOKIE_SIZE_THRESHOLD_) / 40;

/**
 * Given an array of numbers, returns their average rounded to an integer.
 */
var average = function(aNumbers) {
  var total = 0;
  for (var i = 0, len = aNumbers.length; i < len; i++) {
    total += aNumbers[i];
  }
  return aNumbers.length ? Math.round(total / aNumbers.length) : 0;
};

/**
 * Given an array of numbers, returns the element with the max value.
 */
var max = function(aNumbers) {
  var maxval = 0;
  for (var i = 0, len = aNumbers.length; i < len; i++) {
    maxval = Math.max(maxval, aNumbers[i]);
  }
  return maxval;
};

/**
 * Generate a human-readable string that describes the cookie size for
 * the given domain.
 */
var formatDomainAndSize = function(domain, size) {
  return [domain, ' has a cookie size of ',
          PAGESPEED.Utils.formatBytes(size), '.'].join('');
};

/**
 * Rule to find requests with large cookies.
 * @this PAGESPEED.LintRule
 */
var cookieSizeRule = function() {
  var urls = PAGESPEED.Utils.filterProtocols(PAGESPEED.Utils.getResources());

  // Create a map of domain: [cookie_lengths].
  var cookieLengthsByDomain = {};
  var aAllCookieLengths = [];
  for (var i = 0, len = urls.length; i < len; i++) {
    var domain = PAGESPEED.Utils.getDomainFromUrl(urls[i]);

    var headers = PAGESPEED.Utils.getRequestHeaders(urls[i]);
    var cookie = headers['Cookie'];
    if (!cookie) {
      cookie = PAGESPEED.Utils.getCookieString(urls[i]);
    }

    // Map domains to cookie size.
    if (cookie) {
      aAllCookieLengths.push(cookie.length);
      if (cookieLengthsByDomain[domain]) {
        cookieLengthsByDomain[domain].push(cookie.length);
      } else {
        cookieLengthsByDomain[domain] = [cookie.length];
      }
    }
  }

  // Bail out early if there are no cookies.
  if (aAllCookieLengths.length == 0) {
    this.score = 'n/a';
    return;
  }

  var sortedCookieLengths = [];
  for (var domain in cookieLengthsByDomain) {
    sortedCookieLengths.push({
        domain: domain,
        averageCookieSize: average(cookieLengthsByDomain[domain]),
        maxCookieSize: max(cookieLengthsByDomain[domain])
        });
  }

  var sortByMaxSize = function(a, b) {
    return b.maxCookieSize - a.maxCookieSize;
  };
  sortedCookieLengths.sort(sortByMaxSize);

  // Generate warnings messages.
  var aHugeCookieWarnings = [];
  for (var i = 0, len = sortedCookieLengths.length; i < len; i++) {
    var domain = sortedCookieLengths[i].domain;
    var maxCookieSize = sortedCookieLengths[i].maxCookieSize;
    if (maxCookieSize > MAX_COOKIE_SIZE_THRESHOLD_) {
      aHugeCookieWarnings.push(formatDomainAndSize(domain, maxCookieSize));
    }
  }

  var sortByAverageSize = function(a, b) {
    return b.averageCookieSize - a.averageCookieSize;
  };
  sortedCookieLengths.sort(sortByAverageSize);

  var aWarnings = [];
  for (var i = 0, len = sortedCookieLengths.length; i < len; i++) {
    var domain = sortedCookieLengths[i].domain;
    var averageCookieSize = sortedCookieLengths[i].averageCookieSize;
    var maxCookieSize = sortedCookieLengths[i].maxCookieSize;
    if (averageCookieSize > MIN_COOKIE_SIZE_THRESHOLD_ &&
        averageCookieSize < MAX_COOKIE_SIZE_THRESHOLD_) {
      // Report domains with average cookie size over 400 bytes, but
      // not over 1000 bytes, since those are reported elsewhere.
      aWarnings.push(formatDomainAndSize(domain, averageCookieSize));
    }
  }

  var aOutput = [];
  var iAverageCookieLength = average(aAllCookieLengths);
  aOutput.push('The average cookie size for all requests on this page is ');
  aOutput.push(PAGESPEED.Utils.formatBytes(iAverageCookieLength));
  aOutput.push('.<br/><br/>');

  if (aHugeCookieWarnings.length) {
    // Give a max score of C if any cookie is >1000 bytes.
    this.score = 75;
    aOutput.push('The following domains have a cookie size in excess of ');
    aOutput.push(MAX_COOKIE_SIZE_THRESHOLD_);
    aOutput.push(' bytes. This is harmful because requests ');
    aOutput.push('with cookies larger than 1KB typically cannot fit into a ');
    aOutput.push('single network packet.');
    aOutput.push(PAGESPEED.Utils.formatWarnings(aHugeCookieWarnings));
  }

  if (aWarnings.length) {
    // A 0-400 byte cookie scores 100.
    // 400-550 bytes:   90-100 (A)
    // 550-700 bytes:   80-90  (B)
    // 700-850 bytes:   70-80  (C)
    // 850-1000 bytes:  60-70  (D)
    // >1000 bytes:     0-60   (F)
    this.score -=
        Math.max(0, iAverageCookieLength - MIN_COOKIE_SIZE_THRESHOLD_) /
            BYTES_PER_POINT_;
    aOutput.push('The following domains have an average cookie size in ');
    aOutput.push('excess of ');
    aOutput.push(MIN_COOKIE_SIZE_THRESHOLD_);
    aOutput.push(' bytes. Reducing the size of cookies for ');
    aOutput.push('these domains can reduce the time it takes to send ');
    aOutput.push('requests.');
    aOutput.push(PAGESPEED.Utils.formatWarnings(aWarnings));
  }

  this.warnings = aOutput.join('');
};

PAGESPEED.LintRules.registerLintRule(
  new PAGESPEED.LintRule(
    'Minimize cookie size',
    PAGESPEED.REQUEST_GROUP,
    'request.html#MinimizeCookieSize',
    cookieSizeRule,
    0.75
  )
);

})();  // End closure
