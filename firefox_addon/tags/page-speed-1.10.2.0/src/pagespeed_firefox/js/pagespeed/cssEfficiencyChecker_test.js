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
 * @fileoverview Tests for cssEfficiencyChecker.js. Based on Java
 * code by Ian Flanigan.
 *
 * @author Tony Gentilcore
 */

// Stub out all necessary objects here.
Components = {};
Components.classes = {};
Components.classes['@mozilla.org/preferences-service;1'] = {};
Components.classes['@mozilla.org/consoleservice;1'] = {
  getService: function() {
    return {
      logStringMessage: function() {}
    };
  }
};
Components.interfaces = {};

// Mock out all necessary objects here.
var rule_;
PAGESPEED.LintRules.registerLintRule = function(rule) {
  rule_ = rule;
};

function testCalculateBaseCost() {
  assertEquals(
      'Universal',
      PAGESPEED.CssEfficiencyChecker.calculateBaseCost('*').toString());

  assertEquals(
      'Tag',
      PAGESPEED.CssEfficiencyChecker.calculateBaseCost('p').toString());

  assertEquals(
      'Class',
      PAGESPEED.CssEfficiencyChecker.calculateBaseCost('.x').toString());

  assertEquals(
      'ID',
      PAGESPEED.CssEfficiencyChecker.calculateBaseCost('#x').toString());

  assertEquals(
      'ID',
      PAGESPEED.CssEfficiencyChecker.calculateBaseCost('#x:hover').toString());
}

function testAddSelectorTypeMultiplier() {
  var selectorCost = new PAGESPEED.SelectorCost('div p');
  PAGESPEED.CssEfficiencyChecker.addSelectorTypeMultiplier(selectorCost);
  assertEquals(1, selectorCost.multipliers.length);
  assertEquals('descendant selector', selectorCost.multipliers[0].toString());

  selectorCost = new PAGESPEED.SelectorCost('div > p');
  PAGESPEED.CssEfficiencyChecker.addSelectorTypeMultiplier(selectorCost);
  assertEquals(1, selectorCost.multipliers.length);
  assertEquals('child selector', selectorCost.multipliers[0].toString());

  selectorCost = new PAGESPEED.SelectorCost('div + p');
  PAGESPEED.CssEfficiencyChecker.addSelectorTypeMultiplier(selectorCost);
  assertEquals(1, selectorCost.multipliers.length);
  assertEquals('adjacent selector', selectorCost.multipliers[0].toString());

  selectorCost = new PAGESPEED.SelectorCost('a:hover .foo');
  PAGESPEED.CssEfficiencyChecker.addSelectorTypeMultiplier(selectorCost);
  assertEquals(1, selectorCost.multipliers.length);
  assertEquals('descendant selector', selectorCost.multipliers[0].toString());

  selectorCost = new PAGESPEED.SelectorCost('.foo a:hover');
  PAGESPEED.CssEfficiencyChecker.addSelectorTypeMultiplier(selectorCost);
  assertEquals(1, selectorCost.multipliers.length);
  assertEquals('descendant selector', selectorCost.multipliers[0].toString());
}

function testWarnAboutUniversalSelector() {
  assertEquals(
      100,
      PAGESPEED.CssEfficiencyChecker.getSelectorCostsFromRule(
          ':table')[0].getCost());

  assertEquals(
      100,
      PAGESPEED.CssEfficiencyChecker.getSelectorCostsFromRule(
          '[hidden=\"true\"]')[0].getCost());

  assertEquals(
      100,
      PAGESPEED.CssEfficiencyChecker.getSelectorCostsFromRule(
          '*')[0].getCost());
}

function testWarnAboutTagSelector() {
  assertEquals(
      20,
      PAGESPEED.CssEfficiencyChecker.getSelectorCostsFromRule(
          'td')[0].getCost());

  assertEquals(
      40,
      PAGESPEED.CssEfficiencyChecker.getSelectorCostsFromRule(
          'treeitem > treerow')[0].getCost());

  assertEquals(
      20,
      PAGESPEED.CssEfficiencyChecker.getSelectorCostsFromRule(
          'input[type=\"checkbox\"]')[0].getCost());
}

function testWarnAboutOverlyQualifiedSelectors() {
  assertEquals(
      'tag id', 2,
      PAGESPEED.CssEfficiencyChecker.getSelectorCostsFromRule(
        'td#name')[0].getCost());

  assertEquals(
      'tag class', 2,
      PAGESPEED.CssEfficiencyChecker.getSelectorCostsFromRule(
          'p.title')[0].getCost());

  assertEquals(
      'id class', 2,
      PAGESPEED.CssEfficiencyChecker.getSelectorCostsFromRule(
          '#foo.bar')[0].getCost());

  assertEquals(
      'descendant', 40,
          PAGESPEED.CssEfficiencyChecker.getSelectorCostsFromRule(
            'p.title .class')[0].getCost());
}

function testKeySelectorId() {
  assertEquals(PAGESPEED.SelectorKey.ID,
               PAGESPEED.CssEfficiencyChecker.computeKeySelector(
                 'button#backButton'));
  assertEquals(PAGESPEED.SelectorKey.ID,
               PAGESPEED.CssEfficiencyChecker.computeKeySelector('.foo#bar'));
  assertEquals(PAGESPEED.SelectorKey.ID,
               PAGESPEED.CssEfficiencyChecker.computeKeySelector(
                 '#urlBar[type=\"autocomplete\"]'));
  assertEquals(PAGESPEED.SelectorKey.ID,
               PAGESPEED.CssEfficiencyChecker.computeKeySelector(
                 'treeitem > treerow.foo > treecell#myCell:active'));
}

function testKeySelectorClass() {
  assertEquals(PAGESPEED.SelectorKey.CLASS,
               PAGESPEED.CssEfficiencyChecker.computeKeySelector(
                 'button.toolbarButton'));
  assertEquals(PAGESPEED.SelectorKey.CLASS,
               PAGESPEED.CssEfficiencyChecker.computeKeySelector('#foo.bar'));
  assertEquals(PAGESPEED.SelectorKey.CLASS,
               PAGESPEED.CssEfficiencyChecker.computeKeySelector(
                 '#foo .fancyText'));
  assertEquals(PAGESPEED.SelectorKey.CLASS,
               PAGESPEED.CssEfficiencyChecker.computeKeySelector(
                 'menuitem > .menu-left[checked=\"true\"]'));
}

function testKeySelectorTag() {
  assertEquals(PAGESPEED.SelectorKey.TAG,
               PAGESPEED.CssEfficiencyChecker.computeKeySelector('td'));
  assertEquals(PAGESPEED.SelectorKey.TAG,
               PAGESPEED.CssEfficiencyChecker.computeKeySelector(
                 'treeitem > treerow'));
  assertEquals(PAGESPEED.SelectorKey.TAG,
               PAGESPEED.CssEfficiencyChecker.computeKeySelector(
                 'input[type=\"checkbox\"]'));
}

function testKeySelectorUniversal() {
  assertEquals(PAGESPEED.SelectorKey.UNIVERSAL,
               PAGESPEED.CssEfficiencyChecker.computeKeySelector(':table'));
  assertEquals(PAGESPEED.SelectorKey.UNIVERSAL,
               PAGESPEED.CssEfficiencyChecker.computeKeySelector(
                 '[hidden=\"true\"]'));
  assertEquals(PAGESPEED.SelectorKey.UNIVERSAL,
               PAGESPEED.CssEfficiencyChecker.computeKeySelector('*'));
  assertEquals(PAGESPEED.SelectorKey.UNIVERSAL,
               PAGESPEED.CssEfficiencyChecker.computeKeySelector(
                 'tree > [collapsed=\"true\"]'));
  assertEquals(PAGESPEED.SelectorKey.UNIVERSAL,
               PAGESPEED.CssEfficiencyChecker.computeKeySelector(
                 ':table[hidden=\"true\"]'));
}

function testCatchIdCategorizedRulesWithTagsOrClasses() {
  var selectorCost = new PAGESPEED.SelectorCost('button#backButton');
  PAGESPEED.CssEfficiencyChecker.addOverlyQualifiedSelectorMulipliers(
      selectorCost);
  assertEquals(1, selectorCost.multipliers.length);
  assertEquals('ID overly qualified with tag',
               selectorCost.multipliers[0].toString());

  selectorCost = new PAGESPEED.SelectorCost('.menu-left#newMenuIcon');
  PAGESPEED.CssEfficiencyChecker.addOverlyQualifiedSelectorMulipliers(
      selectorCost);
  assertEquals(1, selectorCost.multipliers.length);
  assertEquals('ID overly qualified with class',
               selectorCost.multipliers[0].toString());
}

function testCatchClassCategorizedRulesWithTags() {
  var selectorCost = new PAGESPEED.SelectorCost('treecell.indented');
  PAGESPEED.CssEfficiencyChecker.addOverlyQualifiedSelectorMulipliers(
      selectorCost);
  assertEquals(1, selectorCost.multipliers.length);
  assertEquals('Class overly qualified with tag',
               selectorCost.multipliers[0].toString());

  selectorCost = new PAGESPEED.SelectorCost('p.bold');
  PAGESPEED.CssEfficiencyChecker.addOverlyQualifiedSelectorMulipliers(
      selectorCost);
  assertEquals(1, selectorCost.multipliers.length);
  assertEquals('Class overly qualified with tag',
               selectorCost.multipliers[0].toString());
}

function testHasTag() {
  assertTrue(PAGESPEED.CssEfficiencyChecker.hasTag('td.foo'));
  assertTrue(PAGESPEED.CssEfficiencyChecker.hasTag('td#foo'));
  assertFalse('class first', PAGESPEED.CssEfficiencyChecker.hasTag('.foo#bar'));
  assertFalse('id first', PAGESPEED.CssEfficiencyChecker.hasTag('#foo.bar'));
}

function testSelectorCostCalculation() {
  var selectorCost = new PAGESPEED.SelectorCost('');
  selectorCost.setBaseCost(new PAGESPEED.Cost(3, 'Base Cost'));

  assertEquals(3, selectorCost.getCost());

  selectorCost.addMultiplier(new PAGESPEED.Cost(7, 'A multiplier'));

  assertEquals(21, selectorCost.getCost());
}

function testTestHoverWithoutLinks() {
  var testHoverWithoutLinks = function(selector) {
    selectorCost = new PAGESPEED.SelectorCost(selector);
    return selectorCost.hasHoverWithoutAnchor();
  }

  assertFalse('No :hover', testHoverWithoutLinks('div'));
  assertFalse('No :hover', testHoverWithoutLinks('a'));

  assertFalse('Has anchor', testHoverWithoutLinks('a:hover'));
  assertFalse('Has anchor', testHoverWithoutLinks('A:hover'));
  assertFalse('Has anchor', testHoverWithoutLinks('div a:hover'));

  assertTrue('No anchor', testHoverWithoutLinks(':hover'));
  assertTrue('No anchor', testHoverWithoutLinks('*:hover'));
  assertTrue('No anchor', testHoverWithoutLinks('div :hover'));

  assertFalse('Class, has anchor', testHoverWithoutLinks('a.class:hover'));
  assertFalse('Class, has anchor', testHoverWithoutLinks('div a.class:hover'));
  assertTrue('Class, no anchor', testHoverWithoutLinks('div.class:hover'));
  assertTrue('Class, no anchor', testHoverWithoutLinks('div h1.class:hover'));

  assertFalse('Attribute, anchor', testHoverWithoutLinks('a[attr]:hover'));
  assertFalse('Attribute, anchor', testHoverWithoutLinks('div a[attr]:hover'));

  assertFalse('~=, anchor', testHoverWithoutLinks('a[class~=\'x\']:hover'));
  assertFalse('decedent, ~=, anchor',
              testHoverWithoutLinks('div a[class~=\'x\']:hover'));

  assertFalse('|=, anchor', testHoverWithoutLinks('a[lang|=\'en\']:hover'));
  assertFalse('decedent, |=, anchor',
              testHoverWithoutLinks('h1 a[lang|=\'en\']:hover'));

  assertTrue('No anchor but starts with a', testHoverWithoutLinks('aaa:hover'));
}
