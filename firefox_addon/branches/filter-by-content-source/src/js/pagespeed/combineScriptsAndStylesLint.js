/**
 * Copyright 2009 Google Inc.
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
 * @fileoverview Checks that there are at most 3 JS/CSS file served
 * from each hostname. We allow 3 because we encourage sites to defer
 * loading of JS/CSS not needed at startup. Thus, sites that adopt
 * this technique would load one JS/CSS at startup, with the minimal
 * code needed to render, and a second JS/CSS in the background after
 * the page load has completed. Thus, the truly optimal number is 2
 * per domain, but we allow 3 in case there are special cases that
 * make this impossible (e.g. one JS file is versioned much more
 * frequently than another).
 *
 * @author Bryan McQuade
 */

(function() {  // Begin closure

// Number of like resources we allow per domain before we start
// reducing the score.
var ALLOWED_RESOURCES_PER_DOMAIN = 2;

/**
 * @param {Object} resourceMap the map from hostname to URLs at that
 *     hostname.
 * @param {String} type the human-readable string name for the type of
 *     resources being processed.
 * @param {Array.<String>} out_aWarnings out param that contains the
 *     list of human-readable warning strings.
 * @return {number} the number of excess resources, summed across all
 *     hostnames in the map.
 */
function processResourceMap(resourceMap, type, out_aWarnings) {
  var totalPenalizedResources = 0;
  for (var hostname in resourceMap) {
    var resources = resourceMap[hostname];
    var numExtraResources = resources.length - ALLOWED_RESOURCES_PER_DOMAIN;
    if (numExtraResources <= 0) {
      continue;
    }
    // Only penalize for 3 or more.
    var numPenalizedResources = numExtraResources - 1;
    out_aWarnings.push(['There are ', resources.length, ' ', type,
                        ' files served from ', hostname,
                        '. They should be combined into as few files ',
                        'as possible.',
                        PAGESPEED.Utils.formatWarnings(resources)]
                        .join(''));
    totalPenalizedResources += numPenalizedResources;
  }

  return totalPenalizedResources;
}

/**
 * @this PAGESPEED.LintRule
 * @param {PAGESPEED.ResourceAccessor} resourceAccessor An object that
 *     allows rules to fetch content by type.
 * @param {string} type The type of resource to score.
 * @param {string} displayName |type| as a human-readable name.
 */
var combineResourcesCommon = function(resourceAccessor, type, displayName) {

  // TODO: Consider warning about many items fetched after onload if they are
  // fetched at roughly the same time.

  var resources = resourceAccessor.getResources(
      type,
      null,  // No extra filter.
      true); // Only return resources fetched before onload.

  if (resources.length == 0) {
    this.score = 'n/a';
    return;
  }

  var aWarnings = [];
  var map = PAGESPEED.Utils.getHostToResourceMap(resources);
  var totalExtraResources = processResourceMap(map, displayName, aWarnings);

  this.score = 100 - (totalExtraResources * 11);
  this.warnings = aWarnings.join('');
};

/**
 * @this PAGESPEED.LintRule
 * @param {PAGESPEED.ResourceAccessor} resourceAccessor An object that
 *     allows rules to fetch content by type.
 */
var combineScriptsLint = function(resourceAccessor) {
  combineResourcesCommon.call(this, resourceAccessor, 'js', 'JavaScript');
};

/**
 * @this PAGESPEED.LintRule
 * @param {PAGESPEED.ResourceAccessor} resourceAccessor An object that
 *     allows rules to fetch content by type.
 */
var combineStylesLint = function(resourceAccessor) {
  combineResourcesCommon.call(this, resourceAccessor, 'css', 'CSS');
};

PAGESPEED.LintRules.registerLintRule(
  new PAGESPEED.LintRule(
    'Combine external JavaScript',
    PAGESPEED.RTT_GROUP,
    'rtt.html#CombineJSandCSS',
    combineScriptsLint,
    3.75,
    'CombineJS'
  )
);

PAGESPEED.LintRules.registerLintRule(
  new PAGESPEED.LintRule(
    'Combine external CSS',
    PAGESPEED.RTT_GROUP,
    'rtt.html#CombineJSandCSS',
    combineStylesLint,
    2.25,
    'CombineCSS'
  )
);

})();  // End closure
