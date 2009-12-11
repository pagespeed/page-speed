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
 * @fileoverview Checks that there are no style blocks in the body.
 *
 * @author Tony Gentilcore
 * @author Bryan McQuade
 */

(function() {  // Begin closure

/**
 * @return {Array} The HTML body nodes of the main document and all
 *     iframes.
 */
function getAllBodyNodes() {
  var aBodyNodes = [];
  var aDocs = PAGESPEED.Utils.getElementsByType('doc');
  if (!aDocs || aDocs.length == 0) {
    return aBodyNodes;
  }

  if (aDocs[0].body) aBodyNodes.push(aDocs[0].body);

  var aFrames = PAGESPEED.Utils.getElementsByType('iframe');
  for (var i = 0, len = aFrames.length; i < len; i++) {
    var frameDocNode = aFrames[i].contentDocument;
    if (!frameDocNode || !frameDocNode.body) continue;
    aBodyNodes.push(frameDocNode.body);
  }

  return aBodyNodes;
}
/**
 * @this PAGESPEED.LintRule
 */
var styleBlocksInBodyLint = function() {
  var aBodyNodes = getAllBodyNodes();
  if (!aBodyNodes || aBodyNodes.length == 0) {
    this.score = 'n/a';
    this.information = 'Could not locate body.';
    return;
  }

  var aWarnings = [];
  for (var i = 0, len = aBodyNodes.length; i < len; i++) {
    var aLocalWarnings = [];

    // 1. Look for inline style blocks.
    var styleNodeList = aBodyNodes[i].getElementsByTagName('style');
    if (styleNodeList.length > 0) {
      aLocalWarnings.push(
          [styleNodeList.length, ' style block(s) in the body ',
           'should be moved to the document head.'].join(''));
      // A small penalty because these only cause a small rendering performance
      // hit. They may also cause reflow and shifting of content.
      this.score -= 6 * styleNodeList.length;
    }

    // 2. Look for <link rel="stylesheet"> nodes.
    var linkNodeList = aBodyNodes[i].getElementsByTagName('link');
    for (var j = 0, jlen = linkNodeList.length; j < jlen; j++) {
      var linkNode = linkNodeList[j];
      if (!linkNode.rel ||
          !linkNode.href ||
          linkNode.rel.toLowerCase() != 'stylesheet') {
        continue;
      }
      aLocalWarnings.push(
          ['Link node ', linkNode.href,
           ' should be moved to the document head.'].join(''));
      // Larger penalty because in IE7 and below, rendering of the entire page
      // is blocked until this has loaded.
      this.score -= 21;
    }

    // 3. If any violations were found, add them to the overall
    // warnings list.
    if (aLocalWarnings.length > 0) {
      aWarnings.push(
          [PAGESPEED.Utils.linkify(aBodyNodes[i].ownerDocument.URL), ': ',
           PAGESPEED.Utils.formatWarnings(aLocalWarnings)].join(''));
    }
  }

  if (aWarnings.length > 0) {
    // If there were warnings, format them.
    this.warnings =
      ['<p>CSS in the document body adversely impacts rendering ',
       'performance.</p>',
       aWarnings.join('')].join('');
  }
};

PAGESPEED.LintRules.registerLintRule(
  new PAGESPEED.LintRule(
    'Put CSS in the document head',
    PAGESPEED.RENDERING_GROUP,
    'rendering.html#PutCSSInHead',
    styleBlocksInBodyLint,
    1.0,
    'CssInHead'
  )
);

})();  // End closure
