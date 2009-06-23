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
 * @fileoverview Checks for 2 common external CSS ordering mistakes that block
 * parallelization:
 *   1. External CSS files in the head should appear above external JS files.
 *   2. Inline script blocks should not appear between external styles and
 *      other resources.
 *
 * These checks only includes the head because an existing rule complains
 * if CSS files are in the body.
 *
 * @author Tony Gentilcore
 */

(function() {  // Begin closure

/**
 * @this PAGESPEED.LintRule
 */
var stylesBeforeScriptsLint = function() {
  // Bail out early if there is no head.
  var sHtml = PAGESPEED.Utils.getDocumentContent();
  var aHeads = PAGESPEED.Utils.getContentsOfAllTags(sHtml, 'head');
  if (!aHeads || aHeads.length != 1) {
    this.score = 'n/a';
    return;
  }

  // Find external CSS and all JS in the head and bail out early if the page
  // doesn't have at least 1 external CSS and some type of script.
  var css = PAGESPEED.Utils.getResources('css');
  var js = PAGESPEED.Utils.getResources('js');
  var aInlineScripts = PAGESPEED.Utils.getContentsOfAllTags(
      aHeads[0], 'script');
  if (!css.length || !(js.length || aInlineScripts.length)) {
    this.score = 'n/a';
    return;
  }

  // We determine the order of each component by finding its position within
  // the head. So first find the top most JS file.
  var jsIndices = [];
  var firstJsFileIndex = aHeads[0].length + 1;
  for (var i = 0, len = js.length; i < len; ++i) {
    var index = aHeads[0].indexOf(PAGESPEED.Utils.getPathFromUrl(js[i]));
    jsIndices.push(index);
    if (index >= 0 && index < firstJsFileIndex) {
      firstJsFileIndex = index;
    }
  }

  // Now find all CSS files which have a higher index than the first JS file.
  var cssIndices = [];
  var aWarnings = [];
  for (var i = 0, len = css.length; i < len; ++i) {
    var index = aHeads[0].indexOf(PAGESPEED.Utils.getPathFromUrl(css[i]));
    cssIndices.push(index);
    if (index >= 0 && index > firstJsFileIndex) {
      aWarnings.push(css[i]);
      this.score -= 11;
    }
  }

  // Find all inline script blocks.
  for (var i = 0; i < aInlineScripts.length; i++) {
    // Its an inline script if there is anything within the script block.
    if (aInlineScripts[i] && aInlineScripts[i].length) {
      // This assumes the contents of each script block is unique.
      var index = aHeads[0].indexOf(aInlineScripts[i]);
      var mostRecentCss = -1;
      for (var j = 0; j < cssIndices.length; j++) {
        if (cssIndices[j] > mostRecentCss && cssIndices[j] < index) {
          mostRecentCss = cssIndices[j];
        }
      }
      var mostRecentJs = -1;
      for (var j = 0; j < jsIndices.length; j++) {
        if (jsIndices[j] > mostRecentJs && jsIndices[j] < index) {
          mostRecentJs = jsIndices[j];
        }
      }
      // If this inline script is immeidately after an external css, complain.
      if (mostRecentCss > mostRecentJs) {
        this.score -= 21;
        this.warnings = PAGESPEED.Utils.formatWarnings([[
            'An inline script block was found in the head between an ',
            'external CSS file and another resource. To allow parallel ',
            'downloading, move the inline script before the external CSS ',
            'file, or after the next resource.'
            ].join('')]);
      }
    }
  }

  // If there were warnings, format them.
  if (aWarnings.length) {
    this.warnings += ['The following external CSS files were included after ',
                      'an external JavaScript file in the document head. To ',
                      'ensure CSS files are downloaded in parallel, always ',
                      'include external CSS before external JavaScript.',
                      PAGESPEED.Utils.formatWarnings(aWarnings)].join('');
  }
};

PAGESPEED.LintRules.registerLintRule(
  new PAGESPEED.LintRule(
    'Optimize the order of styles and scripts',
    PAGESPEED.RTT_GROUP,
    'rtt.html#PutStylesBeforeScripts',
    stylesBeforeScriptsLint,
    3.0
  )
);

})();  // End closure
