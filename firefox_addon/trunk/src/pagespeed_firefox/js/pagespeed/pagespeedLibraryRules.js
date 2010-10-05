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
 * @fileoverview Provides a hook into the Page Speed native library.
 *
 * @author Matthew Steele
 */

(function() {  // Begin closure

/**
 * Create an array representing a list of headers from an object representing
 * those headers.
 * @param {object} headers_object An object mapping header names to values.
 * @return {array} An array of [name,value] pairs.
 */
function translateHeaders(headers_object) {
  var headersList = [];
  for (var key in headers_object) {
    headersList.push([key, headers_object[key]]);
  }
  return headersList;
}

/**
 * Given a format array from the results of PageSpeedRules, produce a formatted
 * HTML string.
 * @param {array} formatArray The format array.
 * @return {string} The resulting raw HTML.
 */
function buildHtml(formatArray) {
  var stringParts = [];
  for (var i = 0, length = formatArray.length; i < length; ++i) {
    var item = formatArray[i];
    if (item.type === "url") {
      stringParts.push('<a href="', item.value,
                       '" onclick="document.openLink(this);return false;">',
                       PAGESPEED.Utils.htmlEscape(
                         item.alt ||
                         PAGESPEED.Utils.getDisplayUrl(item.value)),
                       '</a>');
      if (item.alt) {
        stringParts.push(' or ',
                         '<a href="', item.value,
                         '" onclick="document.saveLink(this);return false;"',
                         '" type="application/octet-stream"',
                         '> Save as</a>');
      }
    } else if (item.type === "str") {
      stringParts.push(PAGESPEED.Utils.htmlEscape(item.value));
    }
  }
  return stringParts.join('');
}

/**
 * Given an array of children of a result object from PageSpeedRules, produce a
 * formatted HTML string.
 * @param {array} children The children objects to format.
 * @param {boolean} opt_grand If true, these are (great-)*grandchildren.
 * @return {string} The resulting raw HTML.
 */
function formatChildren(children, opt_grand) {
  if (!children) {
    return null;
  }
  var warnings = [];
  for (var i = 0; i < children.length; ++i) {
    var child = children[i];
    var warning = buildHtml(child.format);
    if (child.children) {
      warning += ' ' + formatChildren(child.children, true);
    }
    warnings.push(warning);
  }
  if (opt_grand) {
    return PAGESPEED.Utils.formatWarnings(warnings, /*allow-raw-html*/true);
  } else {
    return warnings.join('\n<p>\n');
  }
}

// See http://code.google.com/p/page-speed/wiki/BeaconDocs
var shortNameTranslationTable = {
  AvoidBadRequests: 'BadReqs',
  AvoidCssImport: 'CssImport',
  AvoidDocumentWrite: 'DocWrite',
  CombineExternalCss: 'CombineCSS',
  CombineExternalJavaScript: 'CombineJS',
  EnableGzipCompression: 'Gzip',
  LeverageBrowserCaching: 'BrowserCache',
  MinifyCss: 'MinifyCSS',
  MinifyHTML: 'MinifyHTML',
  MinifyJavaScript: 'MinifyJS',
  MinimizeDnsLookups: 'MinDns',
  MinimizeRedirects: 'MinRedirect',
  MinimizeRequestSize: 'MinReqSize',
  OptimizeImages: 'OptImgs',
  OptimizeTheOrderOfStylesAndScripts: 'CssJsOrder',
  ParallelizeDownloadsAcrossHostnames: 'ParallelDl',
  PreferAsyncResources: 'PreferAsync',
  PutCssInTheDocumentHead: 'CssInHead',
  RemoveQueryStringsFromStaticResources: 'RemoveQuery',
  ServeResourcesFromAConsistentUrl: 'DupeRsrc',
  ServeScaledImages: 'ScaleImgs',
  ServeStaticContentFromACookielessDomain: 'NoCookie',
  SpecifyACacheValidator: 'CacheValid',
  SpecifyAVaryAcceptEncodingHeader: 'VaryAE',
  SpecifyCharsetEarly: 'CharsetEarly',
  SpecifyImageDimensions: 'ImgDims',
  SpriteImages: 'Sprite',
};

// New rules that haven't been reviewed by the community are marked as
// experimental. The idea is that rules start as experimental and are
// refined until they graduate to regular rules.
var experimentalRules = {
  AvoidDocumentWrite: true,
  AvoidCssImport: true,
  SpriteImages: true,
  PreferAsyncResources: true,
};

/**
 * Given a list of result objects from the PageSpeedRules, create an array of
 * objects that look like LintRules.
 * @param {array} results An array of results from PageSpeedRules.
 * @return {array} An array of LintRule-like objects.
 */
function buildLintRuleResults(results) {
  var lintRules = [];
  if (results) {
    for (var i = 0; i < results.length; ++i) {
      var result = results[i];
      lintRules.push({
        name: buildHtml(result.format),
        shortName: shortNameTranslationTable[result.name] || result.name,
        score: result.score,
        weight: 3,
        href: result.url || '',
        warnings: formatChildren(result.children),
        information: null,
        getStatistics: function () { return result.stats || {}; },
        experimental: experimentalRules[result.name] || false,
      });
    }
  }
  return lintRules;
}

/**
 * Returns an integer representing the filter the user has chosen from
 * the Filter Results menu.
 * @return {number} 0 for filter nothing, 1 for filter non-ads, etc.
 */
function filterChoice() {
  var IPageSpeedRules = Components.interfaces['IPageSpeedRules'];
  // This data structure must be kept in sync with
  // cpp/pagespeed/pagespeed_rules.cc::ChoiceToFilter()
  var menuIdToFilterChoice = {
    'psAnalyzeAll': IPageSpeedRules.RESOURCE_FILTER_ALL,
    'psAnalyzeAds': IPageSpeedRules.RESOURCE_FILTER_ONLY_ADS,
    'psAnalyzeTrackers': IPageSpeedRules.RESOURCE_FILTER_ONLY_TRACKERS,
    'psAnalyzeContent': IPageSpeedRules.RESOURCE_FILTER_ONLY_CONTENT
  };
  for (menuId in menuIdToFilterChoice) {
    if (document.getElementById(menuId).hasAttribute('selected')) {
      return menuIdToFilterChoice[menuId];
    }
  }
  PS_LOG("No selected items in Filter Results menu");
  return IPageSpeedRules.RESOURCE_FILTER_ALL;
}

PAGESPEED.NativeLibrary = {

/**
 * Invoke the native library rules and return an array of LintRule-like
 * objects, or return an empty array if the library is unavailable.
 * @return {array} An array of LintRule-like objects.
 */
  invokeNativeLibraryRules: function () {
    var pagespeedRules = PAGESPEED.Utils.CCIN(
      '@code.google.com/p/page-speed/PageSpeedRules;1', 'IPageSpeedRules');
    if (!pagespeedRules) {
      return [];
    }

    var onloadTime = FirebugContext.window.onloadTime;
    var resources = [];
    var bodyInputStreams = [];
    var resourceURLs = PAGESPEED.Utils.getResources();
    for (var i = 0; i < resourceURLs.length; ++i) {
      var url = resourceURLs[i];
      var res_body_index = bodyInputStreams.length;
      var inputStream;
      try {
        inputStream = PAGESPEED.Utils.getResourceInputStream(url);
      } catch (e) {
        PS_LOG('Could not get input stream for "' + url + '": ' +
               PAGESPEED.Utils.formatException(e));
        continue;
      }
      bodyInputStreams.push(inputStream);
      var requestTime = PAGESPEED.Utils.getResourceProperty(url, 'requestTime');
      var javaScriptCalls =
          PAGESPEED.Utils.getResourceProperty(url, 'javaScriptCalls');
      resources.push({
        // TODO Add req_method and req_body.
        req_headers: translateHeaders(PAGESPEED.Utils.getRequestHeaders(url)),
        res_status: PAGESPEED.Utils.getResponseCode(url),
        res_headers: translateHeaders(PAGESPEED.Utils.getResponseHeaders(url)),
        res_body: res_body_index,
        req_url: url,
        req_cookies: PAGESPEED.Utils.getCookieString(url) || '',
        req_lazy_loaded: (requestTime > onloadTime),
        js_calls: javaScriptCalls
      });
    }

    var inputJSON = JSON.stringify(resources);
    var resultJSON = pagespeedRules.computeAndFormatResults(
      inputJSON,
      PAGESPEED.Utils.newNsIArray(bodyInputStreams),
      PAGESPEED.Utils.getDocumentUrl(),
      PAGESPEED.Utils.getElementsByType('doc')[0],
      filterChoice(),
      PAGESPEED.Utils.getOutputDir('page-speed'));
    var results = JSON.parse(resultJSON);
    return buildLintRuleResults(results);
  }

};

})();  // End closure
