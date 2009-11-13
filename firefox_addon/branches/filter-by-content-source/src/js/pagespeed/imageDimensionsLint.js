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
 * @fileoverview Checks that all images have their dimensions specified.
 *
 * @author Tony Gentilcore
 */

(function() {  // Begin closure

var domUtils = null;
try {
  domUtils = FBL.CCSV('@mozilla.org/inspector/dom-utils;1', 'inIDOMUtils');
} catch (e) {
  PS_LOG('ERROR getting inIDOMUtils.\n' +
      'Exception raised when trying: \'' + e + '\'\n');
}

var widthRegexp = /width[^;:]*?:/gim;
var heightRegexp = /height[^;:]*?:/gim;

/**
 * Determines if the given element has width and height HTML attributes set.
 * @param {Object} element The element to check.
 * @return {boolean} false iff given element is missing width or height.
 */
function hasHtmlDimensions(element) {
  return (null != element.attributes.getNamedItem('width') &&
          null != element.attributes.getNamedItem('height'));
}

/**
 * Determines if the given element has width and height set in the HTML style
 * attribute.
 * @param {Object} element The element to check.
 * @return {boolean} false iff given element is missing width or height.
 */
function hasStyleAttributeDimensions(element) {
  if (!element.style || !element.style.cssText) {
    return false;
  }

  return !!(element.style.cssText.match(widthRegexp) &&
            element.style.cssText.match(heightRegexp));
}

/**
 * Determines if the given element has width and height CSS attributes set by
 * a stylesheet.
 * @param {Object} element The element to check.
 * @return {boolean} false iff given element is missing width or height.
 */
function hasCssDimensions(element) {
  var rules;
  try {
    rules = domUtils ? domUtils.getCSSStyleRules(element) : null;
  } catch (exc) {}

  if (!rules) {
    return false;
  }

  var hasWidth = false;
  var hasHeight = false;
  for (var i = 0, len = rules.Count(); i < len; ++i) {
    var rule = FBL.QI(rules.GetElementAt(i),
                      Components.interfaces.nsIDOMCSSStyleRule);

    if (rule.cssText) {
      hasWidth |= !!rule.cssText.match(widthRegexp);
      hasHeight |= !!rule.cssText.match(heightRegexp);
    }
  }

  return hasWidth && hasHeight;
}

/**
 * @this PAGESPEED.LintRule
 * @param {PAGESPEED.ResourceAccessor} resourceAccessor An object that
 *     allows rules to fetch content by type.
 */
var imageDimensionsLint = function(resourceAccessor) {
  // TODO: Update this rule to use |resourceAccessor|.

  if (!domUtils) {
    this.information = ('This rule requires the DOM Inspector extension to ' +
                        'be installed and enabled.');
    this.warnings = '';
    this.score = 'error';
    return;
  }

  var win = PAGESPEED.Utils.getUnwrappedWindow();
  var images = win.document.getElementsByTagName('img');

  // Bail out early if there are no images on the page.
  if (!images.length) {
    this.score = 'n/a';
    return;
  }

  var oWarnings = {};
  for (var i = 0, len = images.length; i < len; ++i) {
    // Do not report images that have no source.
    if (!images[i].src) continue;

    if (images[i].style.position != 'absolute' &&
        !hasHtmlDimensions(images[i]) &&
        !hasStyleAttributeDimensions(images[i]) &&
        !hasCssDimensions(images[i])) {
      // Keep a count of the number of uses without dimensions for each URL.
      if (oWarnings.hasOwnProperty(images[i].src)) {
        ++oWarnings[images[i].src];
      } else {
        oWarnings[images[i].src] = 1;
      }

      // 5 points per violation.
      this.score -= 5;
    }
  }

  var aWarnings = [];
  for (var url in oWarnings) {
    aWarnings.push(url + (oWarnings[url] > 1 ?
                          (' (' + oWarnings[url] + ' uses)') : ''));
  }

  if (aWarnings && aWarnings.length) {
    this.warnings = [
        'A width and height should be specified for all images in order to ',
        'speed up page display. Specifying image dimensions prevents the ',
        'browser from having to re-position the contents of the page.',
        '<br><br>The following image(s) are missing a width and/or height:',
        PAGESPEED.Utils.formatWarnings(aWarnings)].join('');
  }
};

PAGESPEED.LintRules.registerLintRule(
  new PAGESPEED.LintRule(
    'Specify image dimensions',
    PAGESPEED.RENDERING_GROUP,
    'rendering.html#SpecifyImageDimensions',
    imageDimensionsLint,
    1.75,
    'ImgDims'
  )
);

})();  // End closure
