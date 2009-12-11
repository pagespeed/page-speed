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
 * @fileoverview A lint rule which discourages use of css expressions.
 *
 * @author Tony Gentilcore
 */

(function() {  // Begin closure

/**
 * @this PAGESPEED.LintRule
 */
var cssExpressionRule = function() {
  var aWarnings = [];
  var aStyles = PAGESPEED.Utils.getContentsOfAllScriptsOrStyles('style');
  if (aStyles.length == 0) {
    this.score = 'n/a';
    return;
  }
  var numExpressions = 0;
  for (var i = 0, len = aStyles.length; i < len; ++i) {
    if (!aStyles[i].content) continue;
    var m = aStyles[i].content.match(/expression\(/g);
    if (!m) continue;

    // TODO: It would be nice to return the contents of the expression,
    // but this doesn't seem possible with a regex alone.
    aWarnings.push([aStyles[i].name,
                    ' contains ',
                    m.length,
                    ' CSS expression' +
                    (m.length > 1 ? 's' : ''),
                    '.'].join(''));
    numExpressions += m.length;
  }

  if (numExpressions) {
    this.score = 100 - (11 * numExpressions);
    this.warnings = [
      'Found ', numExpressions,
      ' CSS expression', (numExpressions > 1 ? 's': ''),
      '. CSS expressions ',
      'should not be used because they degrade rendering performance ',
      'in Internet Explorer.<br>',
      PAGESPEED.Utils.formatWarnings(aWarnings)].join('');
  }
};

PAGESPEED.LintRules.registerLintRule(
  new PAGESPEED.LintRule(
    'Avoid CSS expressions',
    PAGESPEED.RENDERING_GROUP,
    'rendering.html#AvoidCSSExpressions',
    cssExpressionRule,
    2.75,
    'CssExpr'
  )
);

})();  // End closure
