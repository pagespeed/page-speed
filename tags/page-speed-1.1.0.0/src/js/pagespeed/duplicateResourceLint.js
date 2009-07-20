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
 * @fileoverview Check for identical resources that are served from
 * different URLs.
 *
 * @author Bryan McQuade
 */

(function() {  // Begin closure

var MIN_RESOURCE_SIZE = 100;

/**
 * @this PAGESPEED.LintRule
 */
var duplicateResourceRule = function() {
  var allResources = PAGESPEED.Utils.getResources();
  var hashToResourceMap = {};
  for (var i = 0, len = allResources.length; i < len; i++) {
    var url = allResources[i];
    if (PAGESPEED.Utils.getResourceSize(url) < MIN_RESOURCE_SIZE) {
      // Don't complain for very small resources. Lots of sites and ad
      // services use 1x1 GIFs, for instance, and while we'd like to
      // see them all use the same 1x1 GIF, it isn't practical to
      // expect this. Instead, we look for only large resources that
      // match. When 2 large resources match, it's very likely that
      // something is wrong and needs to be fixed.
      continue;
    }
    var inputStream = PAGESPEED.Utils.getResourceInputStream(url);
    if (!inputStream) continue;
    var hash = PAGESPEED.Utils.getMd5HashForInputStream(inputStream);
    PAGESPEED.Utils.tryToCloseStream(inputStream);
    if (!hash) continue;
    if (!hashToResourceMap[hash]) hashToResourceMap[hash] = [];
    hashToResourceMap[hash].push(url);
  }
  var aWarnings = [];
  var totalNumExtraResources = 0;
  for (var hash in hashToResourceMap) {
    var resources = hashToResourceMap[hash];
    var numExtraResources = resources.length - 1;
    if (numExtraResources > 0) {
      totalNumExtraResources += numExtraResources;
      var resourceSize = PAGESPEED.Utils.getResourceSize(resources[0]);
      aWarnings.push(
          resources.join(', ') +
          [' (', PAGESPEED.Utils.formatBytes(resourceSize), ')'].join(''));
    }
  }
  if (aWarnings.length > 0) {
    this.warnings =
        ['The following resources have identical contents, but ',
         'are served from different URLs. Serve resources from a ',
         'consistent URL to reduce the number of requests and the ',
         'number of bytes transferred.',
         PAGESPEED.Utils.formatWarnings(aWarnings)].join('');
    this.score = 100 - (14 * totalNumExtraResources);
  }
};

PAGESPEED.LintRules.registerLintRule(
  new PAGESPEED.LintRule(
    'Serve resources from a consistent URL',
    PAGESPEED.PAYLOAD_GROUP,
    'payload.html#duplicate_resources',
    duplicateResourceRule,
    3.0
  )
);

})();  // End closure
