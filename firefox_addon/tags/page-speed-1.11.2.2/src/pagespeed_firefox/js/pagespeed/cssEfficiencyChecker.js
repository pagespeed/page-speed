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
 * @fileoverview Calculates an estimated cost for every CSS rule.
 *
 * Based on Java code by Ian Flanigan.
 *
 * This work is based on the documented behavior of Firefox as described on the
 * <a href="http://developer.mozilla.org/en/docs/Writing_Efficient_CSS">Writing
 * Efficient CSS</a> page in the Mozilla Developer Center. If we eventually
 * find out how to do similar calculations for IE6, IE7, and Safari, we will.
 * (Note: As of 23 June 2008, the Firefox documentation is up to date and
 * accurate according to L. David Baron, the maintainer of the CSS part of the
 * rendering engine.)
 * <p>
 * The cost of a rule is the cost of its most expensive selector. (Rules with
 * multiple selectors can be thought of as independent rules.)
 * <p>
 * The cost of a selector is calculated as follows:
 * <ul>
 *   <li>First determine the base cost using the <i>key selector</i>, which is
 *     is one of {@code ID}, {@code CLASS}, {@code TAG}, or {@code UNIVERSAL}.
 *     <ul>
 *       <li>{@code UNIVERSAL} selectors are given a cost of 100 because
 *         they are searched for every DOM element in the tree.
 *       <li>{@code TAG} selectors are given a cost of 20 because they are
 *         searched for each tag of that type in the tree.
 *       <li>{@code CLASS} and {@code ID} selectors are given a cost of 1
 *         because they are only searched for elements that have a given
 *         class or id.
 *     </ul>
 *   <li>Next we multiply that base cost by various factors:
 *     <ul>
 *       <li>Selectors that use the descendant combinator (<i>descendant
 *         selectors</i>) are multiplied by 20 since the browser will walk up
 *         the entire DOM to the root trying to match the selector.  It really
 *         doesn't matter how many selectors there are -- the cost is the same.
 *         (This assumes that it costs 20 times more to walk up the tree than
 *         to do a simple lookup.)
 *       <li>Selectors that use the child or adjacent combinator (<i>child
 *         selecotrs</i>, <i>adjacent selectors</i>) are multiplied by 2 for
 *         each child selector because each represents an additional node that
 *         must be considered when evaluating the rule. Note: If there is a
 *         descendant combinator at all, we count it as a descendant selector
 *         and ignore the child/adjacent combinators.
 *       <li>Selectors that have an id plus a class or tag are multiplied by
 *         2 because they are needlessly requiring extra work -- the id by
 *         itself should be sufficient.
 *       <li>Selectors that have a class and a tag are also multiplied
 *         by 2 for the same reason.
 *     </ul>
 * </ul>
 * This algorithm makes it plain to see that a descendant selector with a
 * universal key selector is the ultimate in bad news at a minimum score of
 * 2000. The runner up is a descendant selector with a tag key selector at a
 * minimum score of 400.
 * <p>
 * TODO: Use current DOM to specify a frequency distribution or costs for
 *     different tags, classes, and ids.
 *
 * @author Tony Gentilcore
 */

(function() {  // Begin closure

PAGESPEED.CssEfficiencyChecker = {};

// The dom-utils library returns each selector in the form 'rule/linenumber'.
// This regexp splits the rule from the line number.
var reSelectorLineNumber = /([^\/]+)\/(\d+)/;

var domUtils = null;
try {
  // The DOM Inspector addon is installed by default in FF2 and is built
  // directly into FF3. Nevertheless, we check for its presence in case it
  // is disabled in FF2.
  domUtils = FBL.CCSV('@mozilla.org/inspector/dom-utils;1', 'inIDOMUtils');
} catch (e) {
  PS_LOG('ERROR getting inIDOMUtils.\n' +
      'Exception raised when trying: \'' + e + '\'\n');
}

/**
 * Returns true iff the given style sheet is an inline block.
 */
function isInlineBlock(sheet) {
  return sheet.ownerNode instanceof HTMLStyleElement;
}

/**
 * Rather than expose the cost value directly to the user, we group everything
 * into the following expense buckets. Each value represents the minimum
 * cost to be included in that bucket.
 * @enum {number}
 */
PAGESPEED.Expense = {
  VERY_INEFFICIENT: 1000,  // 1,000+
  INEFFICIENT: 401,    // 401 - 999
  IGNORED: 0   // <= 400 ingored
};

/**
 * Get's the expense bucket for the given cost.
 * @param {number} cost The cost to calculate.
 * @return {PAGESPEED.Expense} The bucket this falls into.
 */
function getExpenseBucket(cost) {
  for (var expense in PAGESPEED.Expense) {
    if (cost >= PAGESPEED.Expense[expense]) {
      return PAGESPEED.Expense[expense];
    }
  }

  PS_LOG('ERROR couldn\'t find expense for: ' + cost);
  return PAGESPEED.Expense.IGNORED;
}

/**
 * The <i>key</i> for a selector, from most specific to most general.  The
 * key is how Firefox classifies selectors in order to make matching more
 * efficient.  The more-specific the key, the more efficient the matching
 * will be.
 * <p>
 * <strong>Note</strong>: Order is important.
 * @enum {number}
 */
PAGESPEED.SelectorKey = {
  ID: 0,
  CLASS: 1,
  TAG: 2,
  UNIVERSAL: 3
};

/**
 * Map selector key values to selector key names.
 * @param {PAGESPEED.SelectorKey} selectorKey Integer key value.
 * @return {string} The name of key selectorKey.
 */
function selectorKeyName(selectorKey) {
  switch (selectorKey) {
  case PAGESPEED.SelectorKey.ID:
    return 'ID';
  case PAGESPEED.SelectorKey.CLASS:
    return 'Class';
  case PAGESPEED.SelectorKey.TAG:
    return 'Tag';
  case PAGESPEED.SelectorKey.UNIVERSAL:
    return 'Universal';
  }

  PS_LOG('ERROR Unknown selector key: ' + selectorKey);
  return 'Unknown';
}

/**
 * Calculates the cost of a given rule. The rule's cost is added
 * to this instance's list of rule costs.
 *
 * In this context a "rule" refers to all of the selectors used by a given CSS
 * rule. For instance:
 * a { background: white; } -> 'a'
 * a.foo, a b, #my-class { color:red; } -> 'a.foo, a b, #my-class'
 *
 * @param {string} rule The CSS rule to compute cost of.
 * @return {!Array.<PAGESPEED.SelectorCost>} An array of the selectors in this
 *     rule.
 */
PAGESPEED.CssEfficiencyChecker.getSelectorCostsFromRule = function(rule) {
  var ruleCost = [];

  var selectors = rule.split(',');
  for (var i = 0, len = selectors.length; i < len; ++i) {
    var selectorCost = computeSelectorCost(PAGESPEED.Utils.trim(selectors[i]));
    ruleCost.push(selectorCost);
  }

  return ruleCost;
};

/**
 * Calculates the cost of a given selector. This cost includes a base cost
 * plus a number of multipliers.
 *
 * In this context a "selector" refers to just one of the selectors to which
 * a "rule" applies. For instance the following "rules" would yield an array of
 * "selectors", each suitable to be passed to this method:
 * 'a' -> ['a']
 * 'a.foo, a b, #my-class' -> ['a.foo','a b','#my-class']
 *
 * @param {string} selector An individual CSS selector.
 * @return {!PAGESPEED.SelectorCost} The cost of the selector.
 */
function computeSelectorCost(selector) {
  var selectorCost = new PAGESPEED.SelectorCost(selector);

  selectorCost.setBaseCost(
    PAGESPEED.CssEfficiencyChecker.calculateBaseCost(selector));
  PAGESPEED.CssEfficiencyChecker.addSelectorTypeMultiplier(selectorCost);
  PAGESPEED.CssEfficiencyChecker.addOverlyQualifiedSelectorMulipliers(
      selectorCost);

  return selectorCost;
}

/**
 * Calculates the base cost of a given selector depending on what the key is
 * for the selector. See the class documentation for more information.
 *
 * @param {string} selector An individual CSS selector.
 * @return {!PAGESPEED.Cost} The base cost of the selector.
 */
PAGESPEED.CssEfficiencyChecker.calculateBaseCost = function(selector) {
  var selectorKey = PAGESPEED.CssEfficiencyChecker.computeKeySelector(selector);

  var cost;
  switch (selectorKey) {
    case PAGESPEED.SelectorKey.UNIVERSAL:
      cost = 100;
      break;
    case PAGESPEED.SelectorKey.TAG:
      cost = 20;
      break;
    default:
      cost = 1;
  }

  return new PAGESPEED.Cost(cost, selectorKeyName(selectorKey));
};

/**
 * Adds a multiplier to the given {@code PAGESPEED.SelectorCost} based on the
 * complexity of its selector.
 *
 * @param {!PAGESPEED.SelectorCost} selectorCost The cost of the selector so
 *     far.
 */
PAGESPEED.CssEfficiencyChecker.addSelectorTypeMultiplier = function(
    selectorCost) {
  var selector = selectorCost.selector;

  var descendants = selector.split(' ');

  if (descendants.length > 1) {
    // Multiply by 20 for the first, and another 10 for each additional.
    selectorCost.addMultiplier(
        new PAGESPEED.Cost(descendants.length * 10,
                       descendants.length > 2 ?
                           (descendants.length - 1) + ' descendant selectors' :
                           'descendant selector'));
  } else {
    // If there are no descendants, check for child/adjacent selectors.
    var childs = selector.split('>');
    var adjacents = selector.split('+');

    if (childs.length > 1) {
      selectorCost.addMultiplier(
          new PAGESPEED.Cost(childs.length,
                         childs.length > 2 ?
                         (childs.length - 1) + ' child selectors' :
                         'child selector'));
    }

    if (adjacents.length > 1) {
      selectorCost.addMultiplier(
          new PAGESPEED.Cost(adjacents.length,
                         adjacents.length > 2 ?
                         (adjacents.length - 1) + ' adjacent selectors' :
                         'adjacent selector'));
    }
  }
};

/**
 * Adds a multiplier to the given {@code selectorCost} based on the the
 * redundancy of its selector.
 *
 * @param {!PAGESPEED.SelectorCost} selectorCost The cost of the selector
 *     so far.
 */
PAGESPEED.CssEfficiencyChecker.addOverlyQualifiedSelectorMulipliers =
     function(selectorCost) {
  var selector = selectorCost.selector;
  var selectors = selector.split(/>|\+|\s/);

  for (var i = 0, len = selectors.length; i < len; ++i) {
    var selectorKey = computeSelectorKey(selectors[i]);

    // Selectors can only be overly qualified if they have an ID or CLASS
    if (selectorKey == PAGESPEED.SelectorKey.ID ||
        selectorKey == PAGESPEED.SelectorKey.CLASS) {

      // Shouldn't have a TAG too
      if (PAGESPEED.CssEfficiencyChecker.hasTag(selectors[i])) {
        selectorCost.addMultiplier(
            new PAGESPEED.Cost(2, selectorKeyName(selectorKey) +
                           ' overly qualified with tag'));
      }

      // Selectors with an ID shouldn't have CLASS refiners
      if (selectorKey == PAGESPEED.SelectorKey.ID) {
        var refiners = selectors[i].split('.');
        for (var j = 1, jlen = refiners.length; j < jlen; ++j) {
          selectorCost.addMultiplier(
            new PAGESPEED.Cost(2, 'ID overly qualified with class'));
        }
      }

      // Selectors with a CLASS shouldn't have ID refiners
      if (selectorKey == PAGESPEED.SelectorKey.CLASS) {
        var refiners = selectors[i].split('#');
        for (var j = 1, jlen = refiners.length; j < jlen; ++j) {
          selectorCost.addMultiplier(
            new PAGESPEED.Cost(2, 'class overly qualified with ID'));
        }
      }
    }
  }
};

/**
 * Returns the key of the given {@code selector}.  The key is the type of
 * it's right-most <i>selector</i>.  This is described on the
 * <a href="http://developer.mozilla.org/en/docs/Writing_Efficient_CSS">
 * Writing Efficient CSS</a> page in the Mozilla Developer Center.
 *
 * @param {string} selector
 * @return {!PAGESPEED.SelectorKey}
 */
PAGESPEED.CssEfficiencyChecker.computeKeySelector = function(selector) {
  // Find the right-most selector
  var selectors = selector.split(/>|\+|\s/);
  return computeSelectorKey(selectors[selectors.length - 1]);
};

/**
 * Computes the key for the current selector, not counting any others that
 * might be combined using combinators.
 *
 * @param {string} selector
 * @return {!PAGESPEED.SelectorKey}
 */
function computeSelectorKey(selector) {
  var selectorType = PAGESPEED.SelectorKey.UNIVERSAL;

  // Look for CLASS and ID refiners
  if (selector.search(/\.[^#]+$/) >= 0) {
    selectorType = PAGESPEED.SelectorKey.CLASS;
  } else if (selector.search(/#[^.]+$/) >= 0) {
    selectorType = PAGESPEED.SelectorKey.ID;
  } else if (PAGESPEED.CssEfficiencyChecker.hasTag(selector)) {
    selectorType = PAGESPEED.SelectorKey.TAG;
  }

  return selectorType;
}

/**
 * Returns true if the given selector has a tag, not counting "*", the
 * universal tag matcher.
 * @param {string} selector
 * @return {boolean}
 */
PAGESPEED.CssEfficiencyChecker.hasTag = function(selector) {
  // Filter out everything in square brackets or after the colon.
  var strippedSelector = selector.replace(/:.*/g, '').replace(/\[.*\]/g, '');
  return strippedSelector != '*' && strippedSelector != '' &&
      strippedSelector.indexOf('.') != 0 && strippedSelector.indexOf('#') != 0;
};

/**
 * A container for the costs associated with a given selector.
 * @param {string} selector
 * @constructor
 */
PAGESPEED.SelectorCost = function(selector) {
  /**@type {string}*/ this.selector = selector.replace(
      / +>/g, '>').replace(
      /> +/g, '>').replace(
      / +\+/g, '+').replace(
      /\+ +/g, '+').replace(
      / +/g, ' ');
  /**@type {Array.<PAGESPEED.SelectorCost>}*/ this.multipliers = [];
  /**@type {Cost} */ this.baseCost = null;
  /**@type {number} */ this.cost = 0;
};

/**
 * @return {number}
 */
PAGESPEED.SelectorCost.prototype.getCost = function() {
  if (this.cost > 0) {
    return this.cost;
  }

  this.cost = this.baseCost.cost;
  for (var i = 0, len = this.multipliers.length; i < len; ++i) {
    this.cost *= this.multipliers[i].cost;
  }

  return this.cost;
};

/**
 * @return {boolean} Does this selector have a :hover pseudo-selector
 *     applied to a non-anchor element?
 */
PAGESPEED.SelectorCost.prototype.hasHoverWithoutAnchor = function() {
  // If there is no :hover, than this selector is not a match.
  if (this.selector.indexOf(':hover') == -1)
    return false;

  // The following characters are delimiters in the list of elements
  // in a selector: ' ', '>', '+'.  We want the last item in the list,
  // so exclude those characters.
  var elementWithHoverMatch = this.selector.match(/([^ >+]*):hover/);
  if (!elementWithHoverMatch)
    return false;

  var elementWithHover = elementWithHoverMatch[1];

  // Want to see if elementWithHover starts with an A tag.
  // To do this, test that the string starts with [Aa], and
  // is not followed by a letter.
  // Matching strings include:
  // a
  // A
  // a.class
  // a[yada yada]
  if (/^[Aa]($|\W)/.test(elementWithHover))
    return false;

  return true;
};

/**
 * @param {!PAGESPEED.Cost} baseCost
 */
PAGESPEED.SelectorCost.prototype.setBaseCost = function(baseCost) {
  this.cost = 0;
  this.baseCost = baseCost;
};

/**
 * @param {!PAGESPEED.Cost} cost
 */
PAGESPEED.SelectorCost.prototype.addMultiplier = function(cost) {
  this.cost = 0;
  this.multipliers.push(cost);
};

/**
 * @return {string}
 */
PAGESPEED.SelectorCost.prototype.toString = function() {
  var s = ['<code>', this.selector, '</code>&nbsp;&nbsp;&nbsp;&nbsp;',
           '<font color=grey>', this.baseCost, ' key'];
  if (this.multipliers.length) {
    s.push(' with ');
    var s2 = [];
    for (var i = 0, len = this.multipliers.length; i < len; ++i) {
      s2.push(this.multipliers[i]);
    }
    s.push(s2.join(' and '));
  }
  s.push('</font>');
  return s.join('');
};

/**
 * A basic unit of cost that has a value and a reason.
 * @param {number} cost The cost value.
 * @param {string} reason The reason for this cost.
 * @constructor
 */
PAGESPEED.Cost = function(cost, reason) {
  /**@type {number}*/ this.cost = cost;
  /**@type {string}*/ this.reason = reason;
};

/**
 * @return {string}
 */
PAGESPEED.Cost.prototype.toString = function() {
  return this.reason;
};

/**
 * @this PAGESPEED.LintRule
 */
var inefficientCssLint = function() {
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

  // Need to get html panel first, otherwise exception will be generated when
  // getting css panel (inside will access the parentPanel, which is html).
  Firebug.currentContext.getPanel('html');
  var cssPanel = Firebug.currentContext.getPanel('css');

  // Iterate through all stylesheets on this page.
  var currInlineBlockNum = 1;
  var styleSheets = cssPanel.getLocationList();
  if (styleSheets.length == 0) {
    this.score = 'n/a';
    return;
  }

  var totalVeryInefficientRules = 0;
  var totalInefficientRules = 0;
  var totalHoverWithoutAnchorRules = 0;

  for (var i = 0, len = styleSheets.length; i < len; ++i) {
    // Get all rules in the current stylesheet.
    var allAvailableSelectors = cssPanel.getStyleSheetRules(cssPanel.context,
                                                            styleSheets[i]);

    // An array to hold a SelectorCost for every CSS selector.
    var aVeryInefficientRules = [];
    var aInefficientRules = [];
    var aHoverWithoutAnchorRules = [];

    var totalCost = 0;
    for (var j = 0, jlen = allAvailableSelectors.length; j < jlen; ++j) {
      // The @import statements have an undefined id, so we ignore them;
      if (!allAvailableSelectors[j].id) continue;

      // The sel is in for 'rule/linenum', so we split them up here and
      // don't display the line number if it is 1.
      var m = reSelectorLineNumber.exec(allAvailableSelectors[j].id);
      var rule = m[1];
      var lineNum = m[2];

      var selectorCosts =
          PAGESPEED.CssEfficiencyChecker.getSelectorCostsFromRule(rule);
      for (var k = 0, klen = selectorCosts.length; k < klen; ++k) {
        var selectorCost = selectorCosts[k];
        var cost = selectorCost.getCost();
        totalCost += cost;

        var bucket = getExpenseBucket(cost);
        switch (bucket) {
        case PAGESPEED.Expense.VERY_INEFFICIENT:
          aVeryInefficientRules.push(selectorCost);
          this.score -= 5;
          break;
        case PAGESPEED.Expense.INEFFICIENT:
          aInefficientRules.push(selectorCost);
          this.score -= 0.5;
          break;
        }

        // TODO: :hover is only expensive when IE[78] renders a page
        // with a strict doctype.  We could check the doctype, and
        // suppress this warning if it would not put IE[78] in strict
        // mode.  However, sites that serve content by user agent
        // might miss the advice.
        if (selectorCost.hasHoverWithoutAnchor()) {
          aHoverWithoutAnchorRules.push(selectorCost.selector);
        }
      }
    }

    // If this is an inline block, set and increment the block number.
    var blockNum = '';
    if (isInlineBlock(styleSheets[i])) {
      // TODO: currInlineBlock number should be counted per
      // styleSheets[i].href instead of globally. This should be fixed here and
      // in unusedCssLint.js.
      blockNum = ' (inline block #' + (currInlineBlockNum++) + ')';
    }

    totalVeryInefficientRules += aVeryInefficientRules.length;
    totalInefficientRules += aInefficientRules.length;
    totalHoverWithoutAnchorRules += aHoverWithoutAnchorRules.length;

    if (!aVeryInefficientRules.length &&
        !aInefficientRules.length &&
        !aHoverWithoutAnchorRules.length)
      continue;

    /**
     * @param {Array} arr An array.
     * @return {string} formatted string version of the array's length.
     */
    var formatLen = function(arr) {
      return PAGESPEED.Utils.formatNumber(arr.length);
    };

    // Print summary for this block.
    this.warnings += [PAGESPEED.Utils.linkify(styleSheets[i].href), blockNum,
                      ' has ',
                      formatLen(aVeryInefficientRules),
                      ' very inefficient rules, ',
                      formatLen(aInefficientRules),
                      ' inefficient rules, and ',
                      formatLen(aHoverWithoutAnchorRules),
                      ' potentially inefficient uses of :hover out of ',
                      formatLen(allAvailableSelectors),
                      ' total rules.<blockquote>'].join('');

    // Print actual warnings.
    if (aVeryInefficientRules.length) {
      this.warnings = [
          this.warnings,
          'Very inefficient rules (good to fix on any page):',
          PAGESPEED.Utils.formatWarnings(aVeryInefficientRules, true)
          ].join('');
    }

    if (aInefficientRules.length) {
      this.warnings = [
          this.warnings,
          'Inefficient rules (good to fix on interactive pages):',
          PAGESPEED.Utils.formatWarnings(aInefficientRules, true)
          ].join('');
    }

    if (aHoverWithoutAnchorRules.length) {
      this.warnings = [
          this.warnings,
          'Rules that use the :hover pseudo-selector on ',
          'non-anchor elements.  This can cause performance ',
          'problems in Internet Explorer versions 7 and 8 ',
          'when a strict doctype is used.',
          PAGESPEED.Utils.formatWarnings(aHoverWithoutAnchorRules, true)
          ].join('');
    }

    this.warnings += '</blockquote>';
  }

  this.addStatistics_({
    numVeryInefficientRules: totalVeryInefficientRules,
    numInefficientRules: totalInefficientRules,
    numHoverWithoutAnchorRules: totalHoverWithoutAnchorRules
  });
};

PAGESPEED.LintRules.registerLintRule(
  new PAGESPEED.LintRule(
    'Use efficient CSS selectors',
    PAGESPEED.RENDERING_GROUP,
    'rendering.html#UseEfficientCSSSelectors',
    inefficientCssLint,
    0.75,
    'CssSelect'
  )
);

})();  // End closure
