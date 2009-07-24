/**
 * Copyright 2009 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * @fileoverview Create an object that holds page speed results.
 *
 * @author Sam Kerner
 */

(function() {  // Begin closure

/**
 * Build an object that holds the results of page speed.
 * This object has the information necessary to write results
 * to a file and send a beacon with scoring data.  Code that
 * outputs page speed results should read from this object,
 * rather than fetching the information itself.  This avoids
 * recomputing information, and decouples code that reports
 * results from the implementation of page speed.  For example,
 * if we change the component collector's API, than we will
 * need to update code in this file, rather than code in the
 * beacon generator, html file writer, and any other user of
 * this class.
 * @param {Object} browserTab The object which represents the
 *     tab scores were computed in.
 * @param {number} overallScore The overall score of the page.
 * @constructor
 */
PAGESPEED.ResultsContainer = function(browserTab, overallScore) {
  this.browserTab_ = browserTab;

  var win = browserTab.contentWindow;
  var lintRules = PAGESPEED.LintRules.lintRules;

  // results_ holds all information this class will store,
  // in a format that can be JSON.stringify()ed.
  this.results_ = {
    resultsFormatVersion: 1  // Increment this number when the format changes.
  };

  // Store versions of pagespeed and anything it depends on.
  this.results_.versions = {
    pageSpeed: PAGESPEED.Utils.getPageSpeedVersion(),
    firebug: PAGESPEED.DEPENDENCIES['Firebug']['installedVersion'],
    firefox: PAGESPEED.Utils.getFirefoxVersion(),
    userAgent: win.navigator.userAgent
  };

  // Store statistics about the page:
  var url = browserTab.currentURI.spec;
  this.results_.pageStats = {
    url: url,
    initialUrl: PAGESPEED.Utils.findPreRedirectUrl(url),
    pageLoadTime: PAGESPEED.PageLoadTimer.getPageLoadTimeByTab(browserTab),
    numRequests: PAGESPEED.Utils.getTotalRequests(),
    pageSize: PAGESPEED.Utils.getTotalResourceSize(),
    transferSize: PAGESPEED.Utils.getTotalTransferSize(),
    dateTime: (new Date).toString()
  };

  // For each lint rule, save an object holding the rule name, score,
  // and any other rule-specific information.
  var ruleResults = [];
  for (var i = 0, len = lintRules.length; i < len; i++) {
    var lintRule = lintRules[i];
    var lintRuleInfo = {
      // TODO: Consider creating an enum to encode rule names.
      // This will make them resistant to renaming, easier to encode,
      // and take fewer bytes when sending a beacon.
      name: lintRule.name,
      score: lintRule.score
      // TODO: Consider adding the html text of the rule.
      // TODO: Add stats gathered by the rule, such as # bytes sent,
      // # extra RRTs, etc.
    };
    ruleResults.push(lintRuleInfo);
  }
  this.results_.rules = ruleResults;

  // The tags object holds tags.  See PAGESPEED.ResultsContainer.addTags().
  this.results_.tags = {};
};

/**
 * Results can be annotated with tags.  Tags are key-value pairs
 * stored with results to allow tracking of results.  For example,
 * page speed might be run against an internal test version of
 * a site, and also against the production version.  To make it
 * clear which results came from test vs. production, a tag might
 * be added like this:
 *
 *   // In production tests:
 *   var results = PAGESPEED.ResultsContainer(curTab);
 *   results.addTags({build: 'production'});
 *
 *   // In development:
 *   var results = PAGESPEED.ResultsContainer(curTab);
 *   results.addTags({build: 'development',
 *                    changelist: 'r109'});
 *
 * Existing tags are overwritten by new tags with the same name.
 *
 * @param {Object} tags An object whose property names and values
 *    are the key value pairs that should be stored with this
 *    results set.
 * @param {string} opt_prefix String prefix to append to each tag name.
 *    Usefull if you want to avoid name collisions. '' if unset.
 */
PAGESPEED.ResultsContainer.prototype.addTags = function(tags, opt_prefix) {
  PAGESPEED.Utils.copyAllPrimitiveProperties(
      this.results_.tags, tags, opt_prefix);
};

/**
 * JSON-encode the results.
 * @param {number} opt_space If set, use this many spaces to indent.
 */
PAGESPEED.ResultsContainer.prototype.toString = function(opt_space) {
  return JSON.stringify(this.results_, null, opt_space);
};

})();  // End closure
