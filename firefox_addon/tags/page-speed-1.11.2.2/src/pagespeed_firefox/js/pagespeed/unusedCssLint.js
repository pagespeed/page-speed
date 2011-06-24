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
 * @fileoverview Detects CSS selectors that don't match any element in the DOM.
 *
 * @author Tony Gentilcore
 */

(function() {  // Begin closure

var pseudoSelectorRegexp = /:hover|:link|:active|:visited|:focus/;
var selectorLineNumberRegexp = /([^\/]+)\/(\d+)/;

var domUtils = null;
try {
  domUtils = PAGESPEED.Utils.CCSV('@mozilla.org/inspector/dom-utils;1',
                                  'inIDOMUtils');
} catch (e) {
  PS_LOG('ERROR getting inIDOMUtils.\n' +
      'Exception raised when trying: \'' + e + '\'\n');
}

PAGESPEED.UnusedCss = {  // Begin namespace

  /**
   * Returns the CSS rule IDs (selector/line_no) used by all element in the
   * window and sub frames.
   * @param {Object} win A reference to the window contains the elements.
   * @return {Object} An object with a key for every selector used by a node.
   */
  getUsedCssRuleIds: function(win) {
    var usedCssRuleIds = {};

    // Get all nodes in the main document and add them to the result array.
    PAGESPEED.UnusedCss.addDocumentCssRuleIdsToMap(
        win.document, usedCssRuleIds);

    // Loop through any frames adding those nodes to the result array.
    for (var i = 0, len = win.frames.length; i < len; ++i) {
      PAGESPEED.UnusedCss.addDocumentCssRuleIdsToMap(
          win.frames[i].document, usedCssRuleIds);
    }
    return usedCssRuleIds;
  },

  /**
   * Returns the CSS rules that do not appear in usedRuleIds.
   * This filters out @import statements because they have undefined rule IDs.
   * This filters out pseudo selectors because they always appear unused.
   * @param {Object} rules
   * @param {Object} usedRuleIds
   * @return {Array} The unused rules.
   */
  getUnusedCssRules: function(rules, usedRuleIds) {
    var unusedCssRules = [];
    for (var i = 0, len = rules.length; i < len; ++i) {
      var rule = rules[i];
      // Skip rules with undefined IDs (@import statements have this).
      if (!rule.id) continue;
      // Skip rules that are used.
      if (usedRuleIds.hasOwnProperty(rule.id)) continue;
      // Skip pseudo selectors because they always appear unused.
      // TODO: It would be nice to do something more intelligent here.
      if (rule.id.match(pseudoSelectorRegexp)) continue;
      unusedCssRules.push(rule);
    }
    return unusedCssRules;
  },

  /**
   * Returns the size (in bytes) of the given style sheet. This code is strongly
   * based on the private method getStyleSheetCSS() in Firebug's css.js.
   */
  getStyleSheetSize: function(sheet) {
    var size = 0;
    if (PAGESPEED.UnusedCss.isInlineBlock(sheet)) {
      var rulesSize = 0;
      for (var i = 0, len = sheet.cssRules.length; i < len; ++i) {
        rulesSize += sheet.cssRules[i].cssText.length;
      }
      size = sheet.ownerNode.innerHTML.length;
      if (rulesSize > size) {
        // Rules added via style.insertRule() do not show up
        // in the innerHTML. Still look at innerHTML in case
        // it has large comments that are not in in cssRules.
        size = rulesSize;
      }
    } else {
      size = PAGESPEED.Utils.getResourceSize(sheet.href);
    }
    return size;
  },

  /**
   * Returns true iff the given style sheet is an inline block.
   */
  isInlineBlock: function(sheet) {
    return sheet.ownerNode instanceof HTMLStyleElement;
  },

  /**
   * Returns true iff the given style sheet is was created via JavaScript.
   */
  isInlineBlockJavaScriptCreated: function(sheet) {
    // This is one definate case.
    return sheet.ownerNode.innerHTML.length === 0;
  },

  /**
   * Returns the estimated size (in bytes) of the unused rules.
   * The comments and whitespace are divided proportionally between the
   * unused and used rules.
   * @param {Number} size Total bytes for the current style block.
   * @param {Array} rules Array of rules from Firebug:getStyleSheetRules.
   * @param {Array} unusedRules Array of rules from Firebug:getStyleSheetRules.
   */
  getUnusedStyleSheetSize: function(size, rules, unusedRules) {
    if (size == 0) return 0;
    var minifiedSize = PAGESPEED.UnusedCss.getMinifiedSize(rules);
    var unusedMinifiedSize = PAGESPEED.UnusedCss.getMinifiedSize(unusedRules);
    var totalMinifiedSize = minifiedSize + unusedMinifiedSize;

    if (totalMinifiedSize == 0) {
      // If the total minified size is 0, unusedMinifiedSize and
      // minifiedSize will be 0 as well, because the size returned
      // by PAGESPEED.UnusedCss.getMinifiedSize() will be positive.
      // If the total size of the input is 0, the possible savings
      // are the entire input size.  Return here to avoid dividing
      // by zero below.
      return size;
    }

    return unusedMinifiedSize +
        (size - totalMinifiedSize) * unusedMinifiedSize / totalMinifiedSize;
  },

  /**
   * Returns the minified size (in bytes) of an array of CSS rules.
   *
   * This does not consider comments needed for hacks.
   *
   * @param {Array} rules Array of rules from Firebug:getStyleSheetRules.
   * @return {Number} The minified size in bytes.
   */
  getMinifiedSize: function(rules) {
    var totalMinifiedSize = 0;
    for (var i = 0, len = rules.length; i < len; ++i) {
      var rule = rules[i];
      if (!rule || !rule.props || !rule.selector) continue;
      var size = rule.selector.length + 2;  // Add two for the curly braces.
      for (var j = 0, len = rule.props.length; j < len; ++j) {
        var prop = rule.props[j];
        size += prop.name.length + prop.value.length;
        size += 2;  // Add two for ":" and ";"
        if (prop.important) {
          size += 11;  // " !important".length
        }
      }
      if (rule.props.length > 0) {
        size -= 1;  // Do not count the final ";".
      }
      totalMinifiedSize += size;
    }
    return totalMinifiedSize;
  },

  /**
   * Adds a CSS rule ID ("selector/line_no") in the given map for all
   * selectors used by the given document.
   * @param {Object} document The document to get selectors for.
   * @param {Object} map A map object to add keys to.
   */
  addDocumentCssRuleIdsToMap: function(document, map) {
    var allDomNodes = document.getElementsByTagName('*');
    for (var i = 0, len = allDomNodes.length; i < len; ++i) {
      PAGESPEED.UnusedCss.addElementCssRuleIdsToMap(allDomNodes[i], map);
    }
  },

  /**
   * Adds a CSS rule ID ("selector/line_no") in the given map for all
   * selectors used by the given element.
   * @param {Object} element The element to get selectors for.
   * @param {Object} map A map object to add keys to.
   */
  addElementCssRuleIdsToMap: function(element, map) {
    var inspectedRules;
    try {
      inspectedRules = domUtils ? domUtils.getCSSStyleRules(element) : null;
    } catch (exc) {}

    if (inspectedRules) {
      for (var i = 0, len = inspectedRules.Count(); i < len; ++i) {
        var rule = FBL.QI(inspectedRules.GetElementAt(i),
                          Components.interfaces.nsIDOMCSSStyleRule);
        map[rule.selectorText + '/' + domUtils.getRuleLine(rule)] = true;
      }
    }
  }
};  // End namespace: UnusedCss

/**
 * @this PAGESPEED.LintRule
 */
var unusedCssLint = function() {
  if (PAGESPEED.Utils.isUsingFilter()) {
    this.score = 'disabled';
    this.warnings = '';
    this.information =
      ['This rule is not currently able to filter results ',
       'and is not included in the analysis.'].join('');
    return;
  }

  if (!domUtils) {
    this.information = ('This rule requires the DOM Inspector extension to ' +
                        'be installed and enabled.');
    this.warnings = '';
    this.score = 'error';
    return;
  }

  var totalCssSize = 0;
  var totalUnusedCssSize = 0;
  var usedWindowRuleIds = PAGESPEED.UnusedCss.getUsedCssRuleIds(
      Firebug.currentContext.window);

  // Iterate through all stylesheets on this page.
  var inlineBlockNum = 1;

  // Need to get html panel first, otherwise exception will be generated when
  // getting css panel (inside will access the parentPanel, which is html).
  Firebug.currentContext.getPanel('html');
  var cssPanel = Firebug.currentContext.getPanel('css');
  var styleSheets = cssPanel.getLocationList();
  if (styleSheets.length == 0) {
    this.score = 'n/a';
    return;
  }

  var warnings = [];
  for (var i = 0, iLen = styleSheets.length; i < iLen; ++i) {
    var sheet = styleSheets[i];
    if (!sheet) continue;
    var sheetRules = cssPanel.getStyleSheetRules(cssPanel.context, sheet);
    var sheetSize = PAGESPEED.UnusedCss.getStyleSheetSize(sheet);
    var unusedSheetRules = PAGESPEED.UnusedCss.getUnusedCssRules(
        sheetRules, usedWindowRuleIds);
    var unusedSheetSize = PAGESPEED.UnusedCss.getUnusedStyleSheetSize(
        sheetSize, sheetRules, unusedSheetRules);

    // Format the warnings for this stylesheet.
    var ruleWarnings = [];
    for (var j = 0, jLen = unusedSheetRules.length; j < jLen; ++j) {
      // The ruleId format is 'selector/linenum', so we split it here and
      // do not display the line number if it is 0 or 1.
      var m = selectorLineNumberRegexp.exec(unusedSheetRules[j].id);
      var selector = '<code>' + m[1] + '</code>';
      var lineNum = (m[2] == '0' || m[2] == '1') ?
          '' : ' <font color=grey>line ' + m[2] + '</font>';
      ruleWarnings.push(selector + lineNum);
    }
    if (ruleWarnings.length > 0) {
      if (PAGESPEED.UnusedCss.isInlineBlock(sheet)) {
        var javascriptNote = '';
        if (PAGESPEED.UnusedCss.isInlineBlockJavaScriptCreated(sheet)) {
          javascriptNote = ' (JavaScript generated)';
        }
        warnings.push(PAGESPEED.Utils.linkify(sheet.ownerNode.baseURI) +
                      ' inline block #' + (inlineBlockNum++) + javascriptNote);
      } else {
        warnings.push(PAGESPEED.Utils.linkify(sheet.href));
      }
      warnings = warnings.concat(
          ': ',
          PAGESPEED.Utils.formatBytes(unusedSheetSize),
          ' of ',
          PAGESPEED.Utils.formatBytes(sheetSize),
          ' is not used by the current page.',
          PAGESPEED.Utils.formatWarnings(ruleWarnings, true));
    }
    totalCssSize += sheetSize;
    totalUnusedCssSize += unusedSheetSize;
  }
  if (totalUnusedCssSize > 0) {
    var totalUnusedPercent = totalUnusedCssSize / totalCssSize;
    // Adjust the unused percent. Smaller unused sizes adjust the percent down
    // by a multiplier between 0.1 and 1. Larger unused sizes (greater than 75k)
    // adjust the percent up by a multiplier greater than 1. A chart of the log
    // function: http://tinyurl.com/c4kynu
    var percentMultiplier =
        Math.log(Math.max(200, totalUnusedCssSize - 800)) / 7 - 0.6;
    this.score = (1 - totalUnusedPercent * percentMultiplier) * 100;

    if (this.score < 100) {
      this.warnings = [
          PAGESPEED.Utils.formatPercent(totalUnusedPercent),
          ' of CSS (estimated ',
          PAGESPEED.Utils.formatBytes(totalUnusedCssSize),
          ' of ',
          PAGESPEED.Utils.formatBytes(totalCssSize),
          ') is not used by the current page.<br><br>'
      ].concat(warnings).join('');
    }
  }
};

PAGESPEED.LintRules.registerLintRule(
  new PAGESPEED.LintRule(
    'Remove unused CSS',
    PAGESPEED.PAYLOAD_GROUP,
    'payload.html#RemoveUnusedCSS',
    unusedCssLint,
    1.5,
    'UnusedCSS'
  )
);

})();  // End closure
