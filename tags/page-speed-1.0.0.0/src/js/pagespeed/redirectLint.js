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
 */
var redirectRule = function() {
  // Loop through all resources and deduct 6 points per redirect.
  var redirects = PAGESPEED.Utils.getComponents().redirect;
  var aWarnings = [];
  for (var url in redirects) {

    // Chained redirects are listed in the elements array.
    var previousUrl = url;
    for (var i = 0, len = redirects[url].elements.length; i < len; ++i) {
      var thisUrl = redirects[url].elements[i];
      aWarnings.push('From ' + previousUrl + ' to ' + thisUrl);
      previousUrl = thisUrl;
      this.score -= 6;
    }
  }

  if (aWarnings.length) {
    this.warnings = ('Remove the following redirects if possible: ' +
                     PAGESPEED.Utils.formatWarnings(aWarnings));
  }
};

PAGESPEED.LintRules.registerLintRule(
  new PAGESPEED.LintRule(
    'Minimize redirects',
    PAGESPEED.RTT_GROUP,
    'rtt.html#AvoidRedirects',
    redirectRule,
    1.5
  )
);

})();  // End closure
