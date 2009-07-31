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
 * @fileoverview A lint rule for determining if all components are compressed.
 *
 * @author Tony Gentilcore
 */

(function() {  // Begin closure

/**
 * @this PAGESPEED.LintRule
 */
var compressionRule = function() {
  // Loop through all files finding any that aren't compressed.
  var compressedBytes = 0;
  var candidateBytes = 0;
  var urls = PAGESPEED.Utils.getResources('doc', 'css', 'js', 'iframe');
  if (urls.length == 0) {
    this.score = 'n/a';
    return;
  }
  var uncompressedResources = [];
  for (var i = 0, len = urls.length; i < len; ++i) {
    var headers = PAGESPEED.Utils.getResponseHeaders(urls[i]);
    var size = PAGESPEED.Utils.getResourceSize(urls[i]);
    // Don't suggest compressing very small components.
    // We consider 150 bytes to be the break-even point for using gzip.
    if (size <= 150) {
      continue;
    }

    candidateBytes += size;
    if (!PAGESPEED.Utils.isCompressed(headers)) {
      uncompressedResources.push(urls[i]);
    } else {
      compressedBytes += size;
    }
  }

  // Sort from largest to smallest.
  var sortBySize = function(a, b) {
    return PAGESPEED.Utils.getResourceSize(b) -
        PAGESPEED.Utils.getResourceSize(a);
  };
  uncompressedResources.sort(sortBySize);

  var aWarnings = [];
  var uncompressedWaste = 0;
  for (var i = 0, len = uncompressedResources.length; i < len; i++) {
    var url = uncompressedResources[i];
    var possibleSavings = 2 * PAGESPEED.Utils.getResourceSize(url) / 3;
    uncompressedWaste += possibleSavings;
    aWarnings.push(
        ['Compressing ', url, ' could save ~',
         PAGESPEED.Utils.formatBytes(possibleSavings), '.'].join(''));
  }

  if (aWarnings.length > 0) {
    this.score = 100 * compressedBytes / candidateBytes;
    this.warnings =
        ['Compressing the following resources with gzip ',
         'could reduce their transfer size by about ',
         'two thirds (~',
          PAGESPEED.Utils.formatBytes(uncompressedWaste), ').',
          PAGESPEED.Utils.formatWarnings(aWarnings)].join('');
  }
};

PAGESPEED.LintRules.registerLintRule(
  new PAGESPEED.LintRule(
    'Enable gzip compression',
    PAGESPEED.PAYLOAD_GROUP,
    'payload.html#GzipCompression',
    compressionRule,
    4.0,
    'Gzip'
  )
);

})();  // End closure
