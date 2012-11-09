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

Components.utils.import('resource://gre/modules/ctypes.jsm');

var MAX_RESPONSE_BODY_SIZE_BYTES = 5 * 1024 * 1024;  // 5MB

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

// Convert to HTML from a list of objects produced by
// FormattedResultsToJsonConverter::ConvertFormattedUrlBlockResults().
function formatUrlBlocks(url_blocks, optimized_content_map) {
  if (!url_blocks) {
    return null;
  }
  var strings = [];
  function formatOptimizedContentIfAny(id) {
    if (typeof(id) !== 'number') {
      return;
    }
    var url = optimized_content_map[id.toString()];
    if (!url) {
      return;
    }
    strings.push('  See <a href="', url,
                 '" onclick="document.openLink(this);return false;">',
                 'optimized version</a> or <a href="', url,
                 '" onclick="document.saveLink(this);return false;"',
                 ' type="application/octet-stream">Save as</a>.');
  }
  for (var i = 0; i < url_blocks.length; ++i) {
    var url_block = url_blocks[i];
    strings.push('<p>');
    strings.push(formatFormatString(url_block.header));
    formatOptimizedContentIfAny(url_block.associated_result_id);
    if (url_block.urls) {
      strings.push('<ul>');
      for (var j = 0; j < url_block.urls.length; ++j) {
        var entry = url_block.urls[j];
        strings.push('<li>', formatFormatString(entry.result));
        formatOptimizedContentIfAny(entry.associated_result_id);
        if (entry.details) {
          strings.push('<ul>');
          for (var k = 0; k < entry.details.length; ++k) {
            strings.push('<li>', formatFormatString(entry.details[k]),
                         '</li>');
          }
          strings.push('</ul>');
        }
        strings.push('</li>');
      }
      strings.push('</ul>');
    }
    strings.push('</p>');
  }
  return strings.join('');
}

// Convert to HTML from an object produced by
// FormattedResultsToJsonConverter::ConvertFormatString().
function formatFormatString(format_string) {
  return format_string.format.replace(/\$[1-9]/g, function (argnum) {
    // The regex above ensures that argnum will be a string of the form "$n"
    // where "n" is a digit from 1 to 9.  We use .substr(1) to get just the "n"
    // part, then use parseInt() to parse it into an integer (base 10), then
    // subtract 1 to get an array index from 0 to 8.  We use this index to get
    // the appropriate argument spec from the format_string.args array.
    var arg = format_string.args[parseInt(argnum.substr(1), 10) - 1];
    // If format_string was constructed wrong, then the index above might have
    // been invalid.  In that case, just leave the "$n" substring as is.
    if (!arg) {
      return argnum;
    }
    // If this argument is a URL, replace the "$n" with a link.  For all other
    // argument types, just replace it with (localized) text.
    if (arg.type === 'url') {
      return ['<a href="', arg.string_value,
              '" onclick="document.openLink(this);return false;">',
              arg.localized_value, '</a>'].join('');
    } else {
      return arg.localized_value;
    }
  });
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

// The (relative) URLs for the docs for each rule:
var documentationURLs = {
  AvoidBadRequests: 'rtt.html#AvoidBadRequests',
  AvoidCharsetInMetaTag: 'rendering.html#SpecifyCharsetEarly',
  AvoidCssImport: 'rtt.html#AvoidCssImport',
  AvoidDocumentWrite: 'rtt.html#AvoidDocumentWrite',
  CombineExternalCss: 'rtt.html#CombineExternalCSS',
  CombineExternalJavaScript: 'rtt.html#CombineExternalJS',
  DeferParsingJavaScript: 'mobile.html#DeferParsingJS',
  EnableGzipCompression: 'payload.html#GzipCompression',
  EnableKeepAlive: 'rtt.html#EnableKeepAlive',
  InlineSmallCss: 'caching.html#InlineSmallResources',
  InlineSmallJavaScript: 'caching.html#InlineSmallResources',
  LeverageBrowserCaching: 'caching.html#LeverageBrowserCaching',
  MakeLandingPageRedirectsCacheable: 'mobile.html#CacheLandingPageRedirects',
  MinifyCss: 'payload.html#MinifyCSS',
  MinifyHTML: 'payload.html#MinifyHTML',
  MinifyJavaScript: 'payload.html#MinifyJS',
  MinimizeDnsLookups: 'rtt.html#MinimizeDNSLookups',
  MinimizeRedirects: 'rtt.html#AvoidRedirects',
  MinimizeRequestSize: 'request.html#MinimizeRequestSize',
  OptimizeImages: 'payload.html#CompressImages',
  OptimizeTheOrderOfStylesAndScripts: 'rtt.html#PutStylesBeforeScripts',
  ParallelizeDownloadsAcrossHostnames: 'rtt.html#ParallelizeDownloads',
  PreferAsyncResources: 'rtt.html#PreferAsyncResources',
  PutCssInTheDocumentHead: 'rendering.html#PutCSSInHead',
  RemoveQueryStringsFromStaticResources: 'caching.html#LeverageProxyCaching',
  ServeResourcesFromAConsistentUrl: 'payload.html#duplicate_resources',
  ServeScaledImages: 'payload.html#ScaleImages',
  SpecifyACacheValidator: 'caching.html#LeverageBrowserCaching',
  SpecifyAVaryAcceptEncodingHeader: 'caching.html#LeverageProxyCaching',
  SpecifyCharsetEarly: 'rendering.html#SpecifyCharsetEarly',
  SpecifyImageDimensions: 'rendering.html#SpecifyImageDimensions',
  SpriteImages: 'rtt.html#SpriteImages',
};

/**
 * Returns an integer representing the filter the user has chosen from
 * the Filter Results menu.
 * @return {number} 0 for filter nothing, 1 for filter non-ads, etc.
 */
function filterChoice() {
  // This data structure must be kept in sync with
  // cpp/pagespeed/pagespeed_rules.cc::ChoiceToFilter()
  var menuIdToFilterChoice = {
    'psAnalyzeAll': 0,
    'psAnalyzeAds': 1,
    'psAnalyzeTrackers': 2,
    'psAnalyzeContent': 3,
  };
  for (menuId in menuIdToFilterChoice) {
    if (document.getElementById(menuId).hasAttribute('selected')) {
      return menuIdToFilterChoice[menuId];
    }
  }
  PS_LOG("No selected items in Filter Results menu");
  return menuIdToFilterChoice['psAnalyzeAll'];
}

function readBinaryDataFromInputStream(inputStream) {
  if (!inputStream) return '';
  var avail = 0;
  try {
    // nsIInputStream.available() will throw if the stream is closed.
    avail = inputStream.available();
  } catch (e) {
    return '';
  }
  // If the response body is too big (e.g. it's a video) then don't
  // bother to read it, since it'll consume too much memory.
  if (avail > MAX_RESPONSE_BODY_SIZE_BYTES) {
    PAGESPEED.Utils.tryToCloseStream(inputStream);
    return '';
  }

  var bStream = PAGESPEED.Utils.CCIN(
      '@mozilla.org/binaryinputstream;1', 'nsIBinaryInputStream');
  bStream.setInputStream(inputStream);
  var data = '';
  try {
    data = bStream.readBytes(bStream.available());
  } catch (e) {
    PS_LOG("Failed to readBytes: " + e);
  }
  PAGESPEED.Utils.tryToCloseStream(bStream);
  PAGESPEED.Utils.tryToCloseStream(inputStream);
  return data;
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

    var resourceURLs = PAGESPEED.Utils.getResources();
    for (var i = 0; i < resourceURLs.length; ++i) {
      var url = resourceURLs[i];
      if (opt_regexp_url_exclude_filter &&
          url.match(opt_regexp_url_exclude_filter)) {
        // This URL matches the URL filter, so it should not be
        // included in the analysis.
        continue;
      }
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
      // For now, we base64-encode all response bodies. We could try
      // to be more clever and only base64-encode if the string
      // contains a non-ASCII byte, but this is simpler at the cost of
      // slightly increased memory usage.
      var responseBody = btoa(readBinaryDataFromInputStream(inputStream));
      var responseBodyEncoding = 'base64';
      var entry = {
        pageref: 'page_0',
        startedDateTime: new Date(requestTime),
        request: {
          method: requestMethod,
          url: url,
          headers: translateHeaders(PAGESPEED.Utils.getRequestHeaders(url)),
        },
        response: {
          httpVersion:  PAGESPEED.Utils.getResponseProtocol(url),
          status: PAGESPEED.Utils.getResponseCode(url),
          headers: translateHeaders(PAGESPEED.Utils.getResponseHeaders(url)),
          content: {
            text: responseBody,
            encoding: responseBodyEncoding,
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
      };
      customEntries.push(custom);
    }

    var har = { log: log };
    var out = { har: har,
                custom: customEntries };
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
    // We use js-ctypes to invoke code in our native library. See
    // https://developer.mozilla.org/en/js-ctypes/Using_js-ctypes for
    // additional details.
    var lib;
    try {
      lib = ctypes.open(PAGESPEED.Utils.getNativeLibraryPath());
    } catch (e) {
    }
    if (!lib) return null;

    var computeAndFormatResults = lib.declare(
        'PageSpeed_ComputeAndFormatResults',
        ctypes.default_abi,
        ctypes.char.ptr,  // return value
        ctypes.char.ptr,  // locale
        ctypes.char.ptr,  // har_data
        ctypes.char.ptr,  // custom_data
        ctypes.char.ptr,  // root_url
        ctypes.char.ptr,  // json_dom
        ctypes.int,       // filter_choice
        ctypes.char.ptr   // output_dir
        );
    var doFree = lib.declare(
        'PageSpeed_DoFree',
        ctypes.default_abi,
        ctypes.void_t,    // return value
        ctypes.char.ptr   // mem
        );
    if (!computeAndFormatResults || !doFree) {
      lib.close();
      return null;
    }

    // Use the browser's locale, not the system locale. Some users
    // have a system locale set up so they can use a different
    // keyboard layout, but they want content presented in a different
    // locale. Since the browser locale is the locale used to populate
    // Accept-Language, it's a strong indicator as to what locale the
    // user wants to read content in.
    var locale = navigator.language;
    var localePref = PAGESPEED.Utils.getStringPref(
        'extensions.PageSpeed.locale');
    if (localePref) {
      // If the user has specified a locale in the preferences, use it
      // instead.
      locale = localePref;
    }
    var documentUrl = PAGESPEED.Utils.getDocumentUrl();
    if (opt_doc) {
      documentUrl = PAGESPEED.Utils.stripUriFragment(opt_doc.URL);
    }
    var input = PAGESPEED.NativeLibrary.constructInputs(
        documentUrl, opt_regexp_url_exclude_filter);

    var serializedDom = JSON.stringify(
        PAGESPEED.Utils.getDocumentDomJson(opt_doc ? opt_doc :
        PAGESPEED.Utils.getElementsByType('doc')[0].get()));

    var outputDir = PAGESPEED.Utils.getOutputDir('page-speed');

    var resultJSON = computeAndFormatResults(
        locale,
        JSON.stringify(input.har),
        JSON.stringify(input.custom),
        documentUrl,
        serializedDom,
        filterChoice(),
        outputDir ? outputDir.path : null);

    var resultJSONStr;
    if (resultJSON.isNull()) {
      PS_LOG('resultJSON is null');
      resultJSONStr = '{}';
    } else {
      resultJSONStr = resultJSON.readString();
    }
    doFree(resultJSON);
    lib.close();
    return JSON.parse(resultJSONStr);
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
    // We use js-ctypes to invoke code in our native library. See
    // https://developer.mozilla.org/en/js-ctypes/Using_js-ctypes for
    // additional details.
    var lib;
    try {
      lib = ctypes.open(PAGESPEED.Utils.getNativeLibraryPath());
    } catch (e) {
    }
    if (!lib) return null;

    var computeResults = lib.declare(
        'PageSpeed_ComputeResults',
        ctypes.default_abi,
        ctypes.char.ptr,  // return value
        ctypes.char.ptr,  // har_data
        ctypes.char.ptr,  // custom_data
        ctypes.char.ptr,  // root_url
        ctypes.char.ptr,  // json_dom
        ctypes.int        // filter_choice
        );
    var doFree = lib.declare(
        'PageSpeed_DoFree',
        ctypes.default_abi,
        ctypes.void_t,    // return value
        ctypes.char.ptr   // mem
        );
    if (!computeResults || !doFree) {
      lib.close();
      return null;
    }

    var documentUrl = PAGESPEED.Utils.getDocumentUrl();
    if (opt_doc) {
      documentUrl = PAGESPEED.Utils.stripUriFragment(opt_doc.URL);
    }
    var input = PAGESPEED.NativeLibrary.constructInputs(
        documentUrl, opt_regexp_url_exclude_filter);
    var serializedDom = JSON.stringify(
        PAGESPEED.Utils.getDocumentDomJson(opt_doc ? opt_doc :
            PAGESPEED.Utils.getElementsByType('doc')[0].get()));
    var resultJSON = computeResults(
        JSON.stringify(input.har),
        JSON.stringify(input.custom),
        documentUrl,
        serializedDom,
        filterChoice());

    var resultJSONStr;
    if (resultJSON.isNull()) {
      PS_LOG('resultJSON is null');
      resultJSONStr = '{}';
    } else {
      resultJSONStr = resultJSON.readString();
    }
    doFree(resultJSON);
    lib.close();
    return JSON.parse(resultJSONStr);
  },

  /**
   * Given a list of result objects from the PageSpeedRules, create an array of
   * objects that look like LintRules.
   * @param {array} results An array of results from PageSpeedRules.
   * @return {array} An array of LintRule-like objects.
   */
  buildLintRuleResults: function(full_results) {
    var results = {
        lintRules: [],
        score: 0,
    };
    if (full_results) {
      results.score = full_results.results.score;
      var rule_results = full_results.results.rule_results;
      for (var i = 0; i < rule_results.length; ++i) {
        var rule_result = rule_results[i];
        results.lintRules.push({
          name: rule_result.localized_rule_name,
          shortName: (shortNameTranslationTable[rule_result.rule_name] ||
                      rule_result.rule_name),
          score: rule_result.rule_score,
          weight: 3,
          rule_impact: rule_result.rule_impact,
          href: documentationURLs[rule_result.rule_name] || '',
          url_blocks: rule_result.url_blocks,
          warnings: formatUrlBlocks(rule_result.url_blocks,
                                    full_results.optimized_content),
          information: null,
          getStatistics: function () { return rule_result.stats || {}; },
          experimental: rule_result.experimental || false,
        });
      }
    }
    return results;
  },
};

})();  // End closure
