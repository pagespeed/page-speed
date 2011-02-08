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
    headersList.push({ name: key, value: headers_object[key] });
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
  InlineSmallCss: 'InlineCSS',
  InlineSmallJavaScript: 'InlineJS',
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
   * Construct the inputs to the Page Speed native library.
   * @param {String} documentUrl The URL of the document being analyzed.
   * @param {RegExp?} opt_regexp_url_exclude_filter An optional URL
   * filter. URLs that match the filter will not be included in the
   * analysis.
   * @return {Object} Object with two properties: har, which contains the HAR,
   * and custom, which contains custom properties.
   */
  constructInputs: function(documentUrl, opt_regexp_url_exclude_filter) {
    var pageLoadStartTime =
        PAGESPEED.Utils.getResourceProperty(documentUrl, 'pageLoadStartTime');
    if (!pageLoadStartTime || pageLoadStartTime <= 0) {
      // Try the requestTime instead.
      pageLoadStartTime =
          PAGESPEED.Utils.getResourceProperty(documentUrl, 'requestTime');
    }
    if (!pageLoadStartTime || pageLoadStartTime <= 0) {
      PS_LOG('Unable to find document request time.');
      return [];
    }
    var startedDateTime = new Date(pageLoadStartTime);

    // onload may not be available in all cases, i.e. if Page Speed
    // was invoked before the onload event.
    var onloadTime =
        PAGESPEED.Utils.getResourceProperty(documentUrl, 'onLoadTime');
    var timeToOnload = -1;
    if (onloadTime && onloadTime > 0) {
      timeToOnload = onloadTime - pageLoadStartTime;
      if (timeToOnload <= 0) {
        PS_LOG('Invalid timeToOnload ' + timeToOnload);
        timeToOnload = -1;
      }
    }

    // We construct a HAR that includes just the subset of data that
    // we need. Some fields required by HAR are excluded if the Page
    // Speed library doesn't require them.
    var log = {};
    log.version = '1.2';
    log.creator = {
      name: 'Page Speed',
      version: PAGESPEED.Utils.getPageSpeedVersion(),
    };
    log.browser = {
      name: 'Firefox',
      version: PAGESPEED.Utils.getFirefoxVersion(),
    };
    log.pages = [
      {
        startedDateTime: startedDateTime,
        id: 'page_0',
        pageTimings: {
          onLoad: timeToOnload,
        },
      },
    ];
    log.entries = [];

    // We also pass a parallel structure that contains extra
    // per-resource data that's not part of the HAR spec.
    var customEntries = [];

    var bodyInputStreams = [];
    var resourceURLs = PAGESPEED.Utils.getResources();
    for (var i = 0; i < resourceURLs.length; ++i) {
      var url = resourceURLs[i];
      if (opt_regexp_url_exclude_filter &&
          url.match(opt_regexp_url_exclude_filter)) {
        // This URL matches the URL filter, so it should not be
        // included in the analysis.
        continue;
      }
      var res_body_index = bodyInputStreams.length;
      var inputStream;
      try {
        inputStream = PAGESPEED.Utils.getResourceInputStream(url);
      } catch (e) {
        PS_LOG('Could not get input stream for "' + url + '": ' +
               PAGESPEED.Utils.formatException(e));
        continue;
      }
      var requestTime = PAGESPEED.Utils.getResourceProperty(url, 'requestTime');
      if (!requestTime || requestTime <= 0) {
        PS_LOG('Could not get request time for "' + url + '".');
        continue;
      }
      var requestMethod =
          PAGESPEED.Utils.getResourceProperty(url, 'requestMethod');
      if (!requestMethod) {
        // This can happen for images that are read out of the image
        // cache. Assume GET for backward compatibility.
        requestMethod = 'GET';
      }
      bodyInputStreams.push(inputStream);
      var entry = {
        pageref: 'page_0',
        startedDateTime: new Date(requestTime),
        request: {
          method: requestMethod,
          url: url,
          headers: translateHeaders(PAGESPEED.Utils.getRequestHeaders(url)),
        },
        response: {
          status: PAGESPEED.Utils.getResponseCode(url),
          headers: translateHeaders(PAGESPEED.Utils.getResponseHeaders(url)),
          content: {
            // Our HAR parser requires this field. We pass the
            // contents outside of the HAR so we just provide an empty
            // body here to make the HAR parser happy. We will
            // overwrite this data with the actual body contents after
            // parsing the HAR.
            text: '',
          },
        },
      };
      log.entries.push(entry);

      // We want to send some per-resource data that isn't part of the
      // HAR spec, so we encode it into an array that parallels the
      // contents of log.entries, and then merge them in the JSON
      // parser.
      var custom = {
        url: url,
        cookieString: PAGESPEED.Utils.getCookieString(url) || '',
        bodyIndex: res_body_index,
        jsCalls:
            PAGESPEED.Utils.getResourceProperty(url, 'javaScriptCalls'),
      };
      customEntries.push(custom);
    }

    var har = { log: log };
    var out = { har: har,
                custom: customEntries,
                bodyInputStreams: bodyInputStreams };
    return out;
  },

  /**
   * Invoke the native library rules and return a structure that
   * contains formatted results, or null if the library is
   * unavailable.
   * @param {nsIDOMDocument?} opt_doc The document to analyze. If
   * null, the root document for the current window is used.
   * @param {RegExp?} opt_regexp_url_exclude_filter An optional URL
   * filter. URLs that match the filter will not be included in the
   * analysis.
   * @return {array} An array of LintRule-like objects.
   */
  invokeNativeLibraryAndFormatResults: function(
      opt_doc, opt_regexp_url_exclude_filter) {
    var pagespeedRules = PAGESPEED.Utils.CCIN(
      '@code.google.com/p/page-speed/PageSpeedRules;1', 'IPageSpeedRules');
    if (!pagespeedRules) {
      return null;
    }

    var documentUrl = PAGESPEED.Utils.getDocumentUrl();
    if (opt_doc) {
      documentUrl = PAGESPEED.Utils.stripUriFragment(opt_doc.URL);
    }
    var input = PAGESPEED.NativeLibrary.constructInputs(
        documentUrl, opt_regexp_url_exclude_filter);
    var resultJSON = pagespeedRules.computeAndFormatResults(
      JSON.stringify(input.har),
      JSON.stringify(input.custom),
      PAGESPEED.Utils.newNsIArray(input.bodyInputStreams),
      documentUrl,
      opt_doc ? opt_doc : PAGESPEED.Utils.getElementsByType('doc')[0],
      filterChoice(),
      PAGESPEED.Utils.getOutputDir('page-speed'));
    return JSON.parse(resultJSON);
  },

  /**
   * Invoke the native library rules and return a JavaScript
   * representation of the Page Speed Results output protocol buffer,
   * or null if the library is unavailable.
   * @param {nsIDOMDocument?} opt_doc The document to analyze. If
   * null, the root document for the current window is used.
   * @param {RegExp?} opt_regexp_url_exclude_filter An optional URL
   * filter. URLs that match the filter will not be included in the
   * analysis.
   * @return {array} An array of LintRule-like objects.
   */
  invokeNativeLibraryAndComputeResults: function(
      opt_doc, opt_regexp_url_exclude_filter) {
    var pagespeedRules = PAGESPEED.Utils.CCIN(
      '@code.google.com/p/page-speed/PageSpeedRules;1', 'IPageSpeedRules');
    if (!pagespeedRules) {
      return null;
    }

    var documentUrl = PAGESPEED.Utils.getDocumentUrl();
    if (opt_doc) {
      documentUrl = PAGESPEED.Utils.stripUriFragment(opt_doc.URL);
    }
    var input = PAGESPEED.NativeLibrary.constructInputs(
        documentUrl, opt_regexp_url_exclude_filter);
    var resultJSON = pagespeedRules.computeResults(
      JSON.stringify(input.har),
      JSON.stringify(input.custom),
      PAGESPEED.Utils.newNsIArray(input.bodyInputStreams),
      documentUrl,
      opt_doc ? opt_doc : PAGESPEED.Utils.getElementsByType('doc')[0],
      filterChoice());
    return JSON.parse(resultJSON);
  },

  /**
   * Given a list of result objects from the PageSpeedRules, create an array of
   * objects that look like LintRules.
   * @param {array} results An array of results from PageSpeedRules.
   * @return {array} An array of LintRule-like objects.
   */
  buildLintRuleResults: function(results) {
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
  },
};

})();  // End closure
