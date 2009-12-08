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
 * @fileoverview Check that the DOM does not have too many elements
 *
 * @author Jason Glasgow
 */

(function() {  // Begin closure

/**
 * TreeWalker filter that visits all elements.
 * @type {nsIDOMTreeWalker}
 */
var acceptAllNodes = {
  acceptNode: function() { return NodeFilter.FILTER_ACCEPT; }
};

/**
 * @return {number} The number of HTML nodes in doc
 */
function countAllNodesInDoc(topDoc) {
  var numNodes = 0;
  var aDocs = [topDoc];
  
  for(var i = 0; i < aDocs.length; i++) {
    var doc = aDocs[i];
    var walker = doc.createTreeWalker(
      doc, NodeFilter.SHOW_ELEMENT, acceptAllNodes, false);
    while (walker.nextNode()) {
      numNodes++;
      var node = walker.currentNode;

      if ((node.tagName.toLowerCase() == 'iframe') && 
          (node.contentDocument != null)) {
        aDocs.push(node.contentDocument);
      }
    }
  }
  return numNodes;
}

/**
 * Turn a number of DOM elements into a score from 0 to 100
 *
 * @param {domElements} int The number of DOM elements
 * @return {number} the score from 0 to 100
 */
function getScoreFromDOMCount(domElements) {

  // A page can have this many DOM elements without any penalty to the score
  var freeDomElements = 1000;

  // A page that reaches this many DOM nodes scores a zero
  var maxDomElements = 4000;

  var score = 100 - 100 * (domElements - freeDomElements) / 
    (maxDomElements - freeDomElements);
  return PAGESPEED.Utils.clampToRange(score, 0, 100);
}

/**
 * @this PAGESPEED.LintRule
 */
var limitDOMElementsRule = function() {

  var aDocs = PAGESPEED.Utils.getElementsByType('doc');
  if (!aDocs || aDocs.length == 0) {
    this.score = 'n/a';
    return;
  }
  var doc = aDocs[0];
  var totalElements = countAllNodesInDoc(doc);

  // Add the number of DOM elements as a statistic so it will be
  // reported via beacons.
  this.addStatistics_({totalDomElements: totalElements});

  // Save the score and a warning to be displayed.
  this.score = getScoreFromDOMCount(totalElements);
  this.warnings = ['This page has ', totalElements, 
                   ' DOM elements. '].join('');
};

PAGESPEED.LintRules.registerLintRule(
  new PAGESPEED.LintRule(
    'Limit number of DOM Elements',
    PAGESPEED.RENDERING_GROUP,
    'rendering.html#LimitDOMElements',
    limitDOMElementsRule,
    1.0,
    'DOMElems'
  )
);

})();  // End closure
