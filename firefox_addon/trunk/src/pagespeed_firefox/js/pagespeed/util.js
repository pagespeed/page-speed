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
 * @fileoverview Provides useful utilities for PAGESPEED.
 *
 * TODO: Break resource handling logic out into a new file.
 *
 * @author Kyle Scholz
 * @author Tony Gentilcore
 * @author Sam Kerner
 * @author Bryan McQuade
 */

Components.utils.import("resource://gre/modules/AddonManager.jsm");

/**
 * Logs the given message to the console.
 * @param {string} msg The message to log.
 */
function PS_LOG(msg) {
  // We set PS_LOG.enabled at the end of this file.
  if (PS_LOG.enabled) {
    var consoleService = Components.classes['@mozilla.org/consoleservice;1']
        .getService(Components.interfaces.nsIConsoleService);
    consoleService.logStringMessage(msg);
  }
  if (PS_LOG.dump) {
    dump(['PS_LOG: ', msg, '\n'].join(''));
  }
}

// TODO(lsong): Copied from chromium extension. Reduce duplication.
function collectElement(element, outList) {
  var obj = {tag: element.tagName};
  // If the tag has any attributes, add an attribute map to the output object.
  var attributes = element.attributes;
  if (attributes && attributes.length > 0) {
    obj.attrs = {};
    for (var i = 0, len = attributes.length; i < len; ++i) {
      var attribute = attributes[i];
      obj.attrs[attribute.name] = attribute.value;
    }
  }
  // If the tag has any attributes, add children list to the output object.
  var children = element.children;
  if (children && children.length > 0) {
    for (var j = 0, len = children.length; j < len; ++j) {
      collectElement(children[j], outList);
    }
  }
  // If the tag has a content document, add that to the output object.
  if (element.contentDocument) {
    obj.contentDocument = collectDocument(element.contentDocument);
  }
  // If this is an IMG tag, record the size to which the image is scaled.
  if (element.tagName === 'IMG' && element.complete) {
    obj.width = element.width;
    obj.height = element.height;
  }
  outList.push(obj);
}

function collectDocument(doc) {
  var elements = [];
  collectElement(doc.documentElement, elements);
  return {
    documentUrl: doc.URL,
    baseUrl: doc.baseURI,
    elements: elements
  };
}

// Define a namespace.
var PAGESPEED = {};

(function() {  // Begin closure

// This GUID string is from install.rdf.  If the id is ever changed,
// then this constant must be updated to match.
var PAGESPEED_GUID_ = '{e3f6c2cc-d8db-498c-af6c-499fb211db97}';

var MAX_FILEPATH_LENGTH = 255;

PAGESPEED.isHealthy = true;

// Fetch the root path of the Page Speed Add-On from the Add-On
// manager. Do this at startup and cache it.
var ADDON_ROOT = null;
AddonManager.getAddonByID(PAGESPEED_GUID_, function(addon) {
    if (addon && addon.getResourceURI) {
      ADDON_ROOT = addon.getResourceURI('.').spec;
    }
  });

/**
 * Dependencies on other Firefox extensions
 */
PAGESPEED.NUM_DEPENDENCIES = 1;
PAGESPEED.DEPENDENCIES = {
  'Firebug': { installedName: 'firebug@software.joehewitt.com',
               url: 'http://getfirebug.com/',
               namespace: 'FBL',
               minimumVersion: '1.7.0',
               maximumVersion: '1.8'
  }
};

// Regular expressions used by util functions are extracted so that they only
// need to be compiled once instead of every time the function is called.
var reExtractDomain = /https?:\/\/([^\/]+)/;
var reExtractPath = /https?:\/\/[^\/]+\/(.*)/;

// This regexp is taken from apendix B of RFC 2396, available at
// http://www.ietf.org/rfc/rfc2396.txt .  Changes made for our use case:
// * URLs must have a protocol in {http, https, file}.
// * no whitespace allowed in most fields.
// * URLs can start at any word break in the string.
var reExtractUrl =
    /(\b((http|https|file):)(\/\/([^\/?#\s]*))?([^?#\s]*)(\?([^\s#]*))?(#([^\s]*))?)/g;
var reUrlFragment = /#.*$/;
var reUrlFragmentOrQuerystring = /[#?].*$/;
var reInvalidVersion = /0(\d+)/g;
var reFourSpaces = /^\s{4}/mg;
var reHttp = /^http/;
var reAmp = /&/g;
var reLt = /</g;
var reGt = />/g;
var reCr = /\r/g;
var reDecimal = /(\d+)(\d{3})/;
var reLeadingSpaces = /^\s*/;
var reTrailingSpaces = /\s*$/;
var reHttpStatusLine = /(^HTTP\/\S+)\s+(\d+)\s+[\w\s]+$/;

// This preference defines the path where optimized output is written.
var OPTIMIZED_FILE_BASE_DIR = 'extensions.PageSpeed.optimized_file_base_dir';

PAGESPEED.Utils = {  // Begin namespace
  /**
   * The key to the Request Header field in Net Panel.
   * @type {string}
   */
  NETPANEL_REQUEST_HEADER: 'requestHeaders',

  /**
   * The key to the Response Header field in Net Panel.
   * @type {string}
   */
  NETPANEL_RESPONSE_HEADER: 'responseHeaders',

  /**
   * The key to the Response Header field in a cache descriptor.
   * @type {string}
   */
  CACHE_RESPONSE_HEADER: 'response-head',

  /**
   * The name of the transferSize field.
   * @type {string}
   */
  TRANSFER_SIZE: 'transferSize',

  /**
   * The name of the resourceSize field.
   * @type {string}
   */
  RESOURCE_SIZE: 'resourceSize',

  /**
   * The name of the response status line field.
   * @type {string}
   */
  RESPONSE_STATUS_LINE: 'responseStatusLine',

  /**
   * The name of the response protocol field.
   * @type {string}
   */
  RESPONSE_PROTOCOL: 'responseProtocol',

  /**
   * The name of the response code field.
   * @type {string}
   */
  RESPONSE_CODE: 'responseCode',

  /**
   * The name of the response from cache field.
   * @type {string}
   */
  RESPONSE_FROM_CACHE: 'fromCache',

  /**
   * The name of the response body field.
   * @type {string}
   */
  RESPONSE_BODY: 'responseBody',

  /**
   * The name of the response content length field.
   * @type {string}
   */
  RESPONSE_CONTENT_LENGTH: 'responseContentLength',

  /**
   * The name of the response storage stream field.
   * @type {string}
   */
  RESPONSE_STORAGE_STREAM: 'responseStorageStream',

  /**
   * Color codes associated with a rule score (returned from getColorCode).
   * @type {number}
   */
  SCORE_CODE_RED: 1,
  SCORE_CODE_YELLOW: 2,
  SCORE_CODE_GREEN: 3,
  SCORE_CODE_INFO: 4,

  /**
   * Get a handle to a service.
   * @param {string} className The class name.
   * @param {string} interfaceName The interface name.
   */
  CCSV: function(className, interfaceName) {
    var classObj = Components.classes[className];
    var ifaceObj = Components.interfaces[interfaceName];
    if (!classObj || !ifaceObj) {
      return null;
    }
    return classObj.getService(ifaceObj);
  },

  /**
   * Get a new instance of a component.
   * @param {string} className The class name.
   * @param {string} interfaceName The interface name.
   */
  CCIN: function(className, interfaceName) {
    var classObj = Components.classes[className];
    var ifaceObj = Components.interfaces[interfaceName];
    if (!classObj || !ifaceObj) {
      return null;
    }
    return classObj.createInstance(ifaceObj);
  },

  /**
   * Create an nsIArray from a Javascript array.
   * @param {Array} jsArray The Javascript array object.
   * @return {nsIArray} A new nsIArray object with the contents of jsArray.
   */
  newNsIArray: function (jsArray) {
    var nsArray = PAGESPEED.Utils.CCIN('@mozilla.org/array;1',
                                       'nsIMutableArray');
    if (nsArray) {
      for (var i = 0, length = jsArray.length; i < length; ++i) {
        // The false below indicates that the reference should not be weak.
        nsArray.appendElement(jsArray[i], false);
      }
    } else {
      PS_LOG("Unable to create nsIMutableArray.");
    }
    return nsArray;
  },

  /**
   * Reads a file from the chrome directory and returns its contents.
   * @param {string} extName Name of the extension the file was installed with.
   * @param {string} fileName Name of the file to read.
   */
  readChromeFile: function(extName, fileName) {
    var url = ['chrome://', extName, '/content/', fileName].join('');
    var ioChannel = PAGESPEED.Utils.getChannelFromIOService(url);
    var stream = ioChannel.open();
    return PAGESPEED.Utils.readInputStream(stream) || '';
  },

  /**
   * Given a URL, returns the domain portion of that URL.
   */
  getDomainFromUrl: function(sUrl) {
    if (!sUrl) return '';
    var domain = sUrl.match(reExtractDomain);
    return (domain && domain[1]) ? domain[1] : '';
  },

  /**
   * Given a URL, returns the path portion of that URL (including querystring).
   */
  getPathFromUrl: function(sUrl) {
    if (!sUrl) return '';
    var path = sUrl.match(reExtractPath);
    return (path && path[1]) ? path[1] : '';
  },

  /**
   * Given a list of resource URLs, returns an object that is a map
   * from hostname to all URLs at that hostname. For instance, if the
   * input contains http://www.example.com/foo,
   * http://www.example.com/asdf, http://foo.example.com/asdf, this
   * method would return a map with two entries, one for
   * www.example.com, and another for foo.example.com. The first entry
   * would be an array with two elements (the two URLs at that
   * hostname), and the second entry would contain an array with one
   * element (the single URL at that hostname).
   */
  getHostToResourceMap: function(aUrls) {
    var hostsToResources = {};
    for (var i = 0, len = aUrls.length; i < len; i++) {
      var url = aUrls[i];
      var hostname = PAGESPEED.Utils.getDomainFromUrl(url);
      if (!hostsToResources[hostname]) {
        hostsToResources[hostname] = [];
      }
      hostsToResources[hostname].push(url);
    }
    return hostsToResources;
  },

  /**
   * Replaces all urls in the given string with hyperlinks that open in a new
   * window or new tab depending on the user's preferences.
   * @param {string} s The string to linkify.
   */
  linkify: function(s) {
    if (!s) return '';
    /**
     * @param {string} matchStr The full regexp match.
     * @param {string} fullUrl The filename portion of the regexp
     *     match.
     * @return {string} The replacement string for the given match.
     */
    var createLinkFromUrl = function(matchStr, fullUrl) {
      var MAX_LINK_TEXT_LEN = 50;
      if (!fullUrl || fullUrl.length == 0) {
        return matchStr;
      }

      // Strip punctuation.  This is a heuristic.  'http:www.foo.com/.' might be
      // a real URL, but we assume the user intends '.' to be punctuation rather
      // than part of the link.
      var nonLinkText = '';
      var reTrailingPunctuation = /([,.])$/;
      var trailingPunctuationMatch = fullUrl.match(reTrailingPunctuation);
      if (trailingPunctuationMatch) {
        nonLinkText = trailingPunctuationMatch[0];
        fullUrl = fullUrl.replace(reTrailingPunctuation, '');
      }

      // Remove any trailing '/'.  'http://foo.com/' -> 'http://foo.com'.
      fullUrl = fullUrl.replace(/\/$/, '');

      return ['<a href="',
              fullUrl,
              '" onclick="document.openLink(this);return false;">',
              PAGESPEED.Utils.getDisplayUrl(fullUrl),
              '</a>',
              nonLinkText].join('');
    };
    return s.replace(reExtractUrl, createLinkFromUrl);
  },

  /**
   * Get a version of the URL that is suitable for display in the UI
   * (~100 characters or less).
   * @param {string} fullUrl The URL to get a display URL for.
   * @return {string} The display URL for this URL.
   */
  getDisplayUrl: function(fullUrl) {
    var MAX_LINK_TEXT_LEN = 100;
    return (fullUrl.length > MAX_LINK_TEXT_LEN ?
            fullUrl.substring(0, MAX_LINK_TEXT_LEN) + '...' :
            fullUrl);
  },

  /**
   * Escapes the given string for HTML rendering.
   */
  htmlEscape: function(s) {
    if (!s) return '';
    return s.replace(reAmp, '&amp;')
        .replace(reLt, '&lt;')
        .replace(reGt, '&gt;');
  },

  /**
   * Get the color code for this rule. The color code is a function of
   * the rule's score and weight.
   * @return {number} 1 for 'red', 2 for 'yellow', 3 for 'green', or 4
   *     for 'info'.
   */
  getColorCode: function(rule) {
    if (rule == null ||
        rule.experimental ||
        isNaN(rule.weight) ||
        rule.weight == 0 ||
        isNaN(rule.score)) {
      // All non-scoring rules, or rules without scores, gen an 'info'
      // color.
      return PAGESPEED.Utils.SCORE_CODE_INFO;
    }
    var impact = rule.rule_impact;
    return (impact < 0 ? PAGESPEED.Utils.SCORE_CODE_INFO :
            impact < 3 ? PAGESPEED.Utils.SCORE_CODE_GREEN :
            impact < 10 ? PAGESPEED.Utils.SCORE_CODE_YELLOW :
            PAGESPEED.Utils.SCORE_CODE_RED);
  },

  /**
   * Returns an array containing an element for each instance of the given
   * tag.  Each element in the array will be a string containing the
   * the contents of that instance of the tag (similar to innerHTML).
   */
  getContentsOfAllTags: function(sHtml, sTag) {
    var aResults = [];
    var aMatches;
    var re = new RegExp('<' + sTag + '[^>]*>([^]*?)(</' + sTag + '>|$)', 'gi');
    while ((aMatches = re.exec(sHtml)) != null) {
      aResults.push(aMatches[1]);
    }
    return aResults;
  },

  /**
   * Formats a given array of warnings into an HTML list for display.
   * @param {Array.<string>} aWarnings An array of warning string.
   * @param {boolean} opt_bRawWarnings If true, the warnings will be passed
   *     through raw without HTML escaping or linkifying.
   * @return {string} The HTML formatted warnings.
   */
  formatWarnings: function(aWarnings, opt_bRawWarnings) {
    if (!aWarnings || aWarnings.length == 0) return '';
    var aFormattedWarnings = ['<ul>'];
    for (var i = 0, len = aWarnings.length; i < len; i++) {
      if (aWarnings[i]) {
        aFormattedWarnings.push(
          '<li>',
          opt_bRawWarnings ? aWarnings[i] :
              PAGESPEED.Utils.linkify(PAGESPEED.Utils.htmlEscape(aWarnings[i])),
          '</li>');
      }
    }
    aFormattedWarnings.push('</ul>');
    return aFormattedWarnings.join('');
  },

  /**
   * Formats a given number of bytes for user display. Examples:
   * "123" -> "123 bytes"
   * "1234" -> "1.2KB"
   * "1234567" -> "1.18MB"
   * "abc" -> "abc"
   *
   * @param {string|number} bytes a number or string of bytes.
   * @return {string} The user formatted number of bytes.
   */
  formatBytes: function(bytes) {
    var iBytes = parseInt(bytes, 10);
    // If it was not successfully parsed, just return the input.
    if (isNaN(iBytes)) {
      return new String(bytes);
    }

    if (iBytes == 0) {
      return '0';
    } else if (iBytes == 1) {
      return '1 byte';
    } else if (iBytes < 1024) {
      return iBytes + ' bytes';
    } else if (iBytes < 1048576) {
      return (Math.round(10 * iBytes / 1024.0) / 10) + 'kB';
    }
    return (Math.round(100 * iBytes / 1048576.0) / 100) + 'MB';
  },

  /**
   * Formats a given number for user display. Example:
   * 123 -> "123"
   * 1234 -> "1,234"
   *
   * @param {number} num The number to format.
   * @return {string} The formatted number.
   */
  formatNumber: function(num) {
    num += '';
    var parts = num.split('.');
    var whole = parts[0];
    var fractional = parts.length > 1 ? '.' + parts[1] : '';
    while (reDecimal.test(whole)) {
      whole = whole.replace(reDecimal, '$1' + ',' + '$2');
    }
    return whole + fractional;
  },

  /**
   * Formats a given percentage for user display. Examples:
   * 0 -> "0%"
   * 0.2 -> "20%"
   * 0.234 -> "23.4%"
   * 0.234567 -> "23.5%"
   * 1.5 -> "150%"
   * @param {number} num The value to format.
   * @return {string} The user formatted percent.
   */
  formatPercent: function(num) {
    if (isNaN(num)) {
      return String(num);
    }
    // Convert to fixed-point Number object instance, then back to a
    // primitive number type. This causes 0.05 to automatically be
    // returned as '5%'. If we didn't convert back to a primitive
    // number type, 0.05 would be returned as '5.0%'.
    var percent = Number(new Number(num * 100).toFixed(1));
    return percent + '%';
  },

  /**
   * If a number is outside a range, change it to be within a range.
   * @param {number} value Value to clip.
   * @param {number} maxValue The largest number allowed.
   * @param {number} minValue The smallest number allowed.
   * @return {number} The value, cliped to the range [minValue, maxValue].
   */
  clampToRange: function(value, minValue, maxValue) {
    if (isNaN(value))
      throw Error('PAGESPEED.Utils.clampToRange() was passed a NaN value.');

    if (value > maxValue)
      return maxValue;

    if (value < minValue)
      return minValue;

    return value;
  },

  /**
   * Trims leading and trailing spaces from the given string.
   * @param {string} input The string to trim.
   * @return {string} The trimmed string.
   */
  trim: function(input) {
    return input.replace(reLeadingSpaces, '').replace(reTrailingSpaces, '');
  },

  /**
   * Returns an array of the types of the given resource URL.
   * @param {string} url The URL of the resource.
   * @return {Array.<string>} An array of the types of this resource.
   */
  getResourceTypes: function(url) {
    var types = [];
    var components = PAGESPEED.Utils.getComponents();
    for (var type in components) {
      // The 'in' opperator below will throw an exception if components[type]
      // is not an object, or if it is null.
      if (!components[type] || typeof(components[type]) != 'object')
        continue;

      if (url in components[type]) {
        types.push(type);
      }
    }
    return types;
  },

  /**
   * Returns a hash of the response headers from the indicated resource.
   * @param {string} url The URL of the resource.
   * @return {Object} A map of response header key/value pairs.
   *
   * This method first attempts to retrieve the headers from the pagespeed
   * cache and if that fails, tries the browser cache.
   *
   * TODO: Add unit tests ASAP.
   */
  getResponseHeaders: function(url) {
    /**
     * Fetch response headers from the cache.
     * @param {string} url The URL to fetch headers for.
     * @param {Object} out_shouldCache Out parameter. If the return
     *     value should be cached by the caller, we set
     *     out_shouldCache.value to true.
     */
    function fetchResponseHeaders(url, out_shouldCache) {
      var cacheEntry = PAGESPEED.Utils.getCacheEntry(url);
      if (!cacheEntry) return null;

      var responseHeaders = cacheEntry.getMetaDataElement(
          PAGESPEED.Utils.CACHE_RESPONSE_HEADER);
      cacheEntry.close();
      if (!responseHeaders) return null;

      responseHeaders = responseHeaders.replace(reCr, '');
      var headerLines = responseHeaders.split('\n');
      var headers = {};
      var headerCount = 0;
      for (var i = 0, len = headerLines.length; i < len; i++) {
        var index = headerLines[i].indexOf(': ');
        if (index != -1) {
          var name = PAGESPEED.Utils.trim(headerLines[i].substring(0, index));
          var value = PAGESPEED.Utils.trim(headerLines[i].substring(index + 1));
          headers[name] = value;
          headerCount++;
        }
      }
      out_shouldCache.value = (headerCount > 0);
      if (out_shouldCache.value) {
        // This response came from the browser cache, so update the
        // fromCache property accordingly. See comment in
        // getFromCache for more details.
        PAGESPEED.Utils.setResourceProperty(
            url, PAGESPEED.Utils.RESPONSE_FROM_CACHE, true);
      }
      return headers;
    }

    var result = PAGESPEED.Utils.getAndCacheResourceProperty(
        url, PAGESPEED.Utils.NETPANEL_RESPONSE_HEADER, fetchResponseHeaders);
    return result || {};
  },

  /**
   * @param {string} url the URL to get a response status line.
   * @return {string} the response status line or null if the response status
   *     couldn't be found.
   */
  getResponseStatusLine: function(url) {
    /**
     * Fetch the response code from the cache.
     * @param {string} url The URL to fetch the response code for.
     * @param {Object} out_shouldCache Out parameter. If the return
     *     value should be cached by the caller, we set
     *     out_shouldCache.value to true.
     */
    function fetchResponseStatusLine(url, out_shouldCache) {
      var cacheEntry = PAGESPEED.Utils.getCacheEntry(url);
      if (!cacheEntry) return null;

      var responseHeaders = cacheEntry.getMetaDataElement(
          PAGESPEED.Utils.CACHE_RESPONSE_HEADER);
      cacheEntry.close();
      if (!responseHeaders) return null;

      responseHeaders = responseHeaders.replace(reCr, '');
      var headerLines = responseHeaders.split('\n');
      if (headerLines.length <= 0) return null;

      // We found the response status line, so tell the caller that
      // it's ok to cache it.
      out_shouldCache.value = true;
      return headerLines[0];
    }
    return PAGESPEED.Utils.getAndCacheResourceProperty(
        url, PAGESPEED.Utils.RESPONSE_STATUS_LINE, fetchResponseStatusLine);

  },

  /**
   * @param {string} url the URL to get a response protocol.
   * @return {number} the response protocol string, or null if the response
   *     protocol couldn't be found.
   */
  getResponseProtocol: function(url) {
    /**
     * Fetch the response protocol from the cache.
     * @param {string} url The URL to fetch the response protocol for.
     * @param {Object} out_shouldCache Out parameter. If the return
     *     value should be cached by the caller, we set
     *     out_shouldCache.value to true.
     */
    function fetchResponseProtocol(url, out_shouldCache) {
      var statusLine =  PAGESPEED.Utils.getResponseStatusLine(url);
      if (statusLine == null) return null;
      var statusCode = statusLine.match(reHttpStatusLine);
      if (statusCode && statusCode.length >= 3) {
        out_shouldCache.value = true;
        return statusCode[1];
      }
      return null;
    }
    return PAGESPEED.Utils.getAndCacheResourceProperty(
        url, PAGESPEED.Utils.RESPONSE_PROTOCOL, fetchResponseProtocol);
  },

  /**
   * @param {string} url the URL to get a response code for.
   * @return {number} the response code, or -1 if the response code
   *     couldn't be found.
   */
  getResponseCode: function(url) {
    /**
     * Fetch the response code from the cache.
     * @param {string} url The URL to fetch the response code for.
     * @param {Object} out_shouldCache Out parameter. If the return
     *     value should be cached by the caller, we set
     *     out_shouldCache.value to true.
     */
    function fetchResponseCode(url, out_shouldCache) {
      var statusLine =  PAGESPEED.Utils.getResponseStatusLine(url);
      if (statusLine == null) return -1;
      var statusCode = statusLine.match(reHttpStatusLine);
      if (statusCode && statusCode.length >= 3) {
        var code = Number(statusCode[2]);
        if (code > 0) {
          // We found a valid response code, so tell the caller that
          // it's ok to cache it.
          out_shouldCache.value = true;
        }
        return code;
      }

      return -1;
    }
    return Number(PAGESPEED.Utils.getAndCacheResourceProperty(
        url, PAGESPEED.Utils.RESPONSE_CODE, fetchResponseCode));
  },

  /**
   * @param {string} url the URL to check.
   * @return {boolean} whether or not the URL was served from cache.
   */
  getFromCache: function(url) {
    // In Firefox 3.5 there is a callback to indicate that a response
    // was read from cache. In Firefox 3.0, however, there is no such
    // callback. In Firefox 3.0, we infer that a response came from
    // cache by checking to see if we were unable to record the response
    // headers. If we were unable to record response headers (meaning
    // we had to read them out of the cache), then it indicates that
    // the response, too, was read from cache.
    PAGESPEED.Utils.getResponseHeaders(url);
    return Boolean(PAGESPEED.Utils.getResourceProperty(
        url, PAGESPEED.Utils.RESPONSE_FROM_CACHE));
  },

  /**
   * Returns a hash of the request headers from the indicated component.
   * @param {string} url The URL of the request.
   * @return {Object} A map of request header key/value pairs.
   *
   * Note: This method does not return headers for images served from
   * the in-memory image cache.
   */
  getRequestHeaders: function(url) {
    // Return the cached value if available
    var headers = PAGESPEED.Utils.getResourceProperty(
        url, PAGESPEED.Utils.NETPANEL_REQUEST_HEADER);
    return headers || {};
  },

  /**
   * @param {string} url The URL to fetch cookies for.
   * @return {string} The cookies for the given URL.
   */
  getCookieString: function(url) {
    var cookieService = PAGESPEED.Utils.CCSV(
        '@mozilla.org/cookieService;1', 'nsICookieService');
    var uri = PAGESPEED.Utils.getIOService().newURI(url, null, null);
    return cookieService.getCookieString(uri, null);
  },

  /**
   * Returns the content of the given resource URL. Do not call this
   *     method for binary resources, since it will truncate the response
   *     data.
   * @param {string} url The URL of the request.
   * @return {string} The content of the resource.
   */
  getResourceContent: function(url) {
    /**
     * Fetch the resource content from the resource's input stream.
     * @param {string} url The URL to fetch headers for.
     * @param {Object} out_shouldCache Out parameter. If the return
     *     value should be cached by the caller, we set
     *     out_shouldCache.value to true.
     */
    function fetchResponseBody(url, out_shouldCache) {
      var inputStream = PAGESPEED.Utils.getResourceInputStream(url);
      var content = PAGESPEED.Utils.readInputStream(inputStream);
      if (!content) content = '';
      out_shouldCache.value = (content.length > 0);
      return content;
    }
    return String(PAGESPEED.Utils.getAndCacheResourceProperty(
        url, PAGESPEED.Utils.RESPONSE_BODY, fetchResponseBody));
  },

  /**
   * Returns an input stream for the given resource URL.
   * @param {string} url The URL of the request.
   * @return {nsIInputStream} An input stream containing the content
   *     of the resource.
   */
  getResourceInputStream: function(url) {
    var types = PAGESPEED.Utils.getResourceTypes(url);
    for (var i = 0, len = types.length; i < len; i++) {
      if (types[i] == 'redirect') {
        return null;
      }
    }
    var storageStream = PAGESPEED.Utils.getResourceProperty(
        url, PAGESPEED.Utils.RESPONSE_STORAGE_STREAM);
    if (storageStream !== undefined) {
      return storageStream.newInputStream(0);
    }

    // Next try to read from the cache. Use the IO service instead of
    // directly accessing the cache, since the IO service
    // will perform all necessary content transformations
    // (decompression, charset, etc).
    var ioChannel = PAGESPEED.Utils.getChannelFromIOService(url);
    ioChannel.loadFlags |=
        // Don't hit the network.
        Components.interfaces.nsICachingChannel.LOAD_ONLY_FROM_CACHE |

        // Tells the channel to disable all validation checks.
        Components.interfaces.nsIRequest.LOAD_FROM_CACHE |

        // Tells the channel to use ACCESS_READ when
        // accessing the cache.
        Components.interfaces.nsIRequest.INHIBIT_CACHING |

        // Use whatever is available in the cache. We don't care
        // about validation.
        Components.interfaces.nsIRequest.VALIDATE_NEVER |

        // If, when the IO service attempts to read the resource
        // from the cache, it gets
        // NS_ERROR_CACHE_WAIT_FOR_VALIDATION,
        // LOAD_BYPASS_LOCAL_CACHE_IF_BUSY tells it to fail the
        // request and return immediately. Otherwise we would block
        // on the validation.
        Components.interfaces.nsICachingChannel.
            LOAD_BYPASS_LOCAL_CACHE_IF_BUSY;

    var stream;
    try {
      stream = ioChannel.open();
    } catch (e) {
      // An exception may be thrown when there is no cache entry, or when
      // opening the cache entry returns
      // NS_ERROR_CACHE_WAIT_FOR_VALIDATION.
    }

    if (stream) return stream;

    PS_LOG('Failed to get input stream for ' + url);
    return null;
  },

  /**
   * Returns a human file name for the given resource URL.
   * @param {string} url The URL of the request.
   * @return {string} Filename from the original URL without extension.
   */
  getHumanFileName: function(url, opt_blockNum) {
    var sanitizePath = function(str) {
      return str.replace(/[^a-zA-Z0-9\.-]/g, '_');
    };

    var queryStart = url.indexOf('?');
    var path = (queryStart != -1) ? url.substring(0, queryStart) : url;
    var originalFileName =
      sanitizePath(path.substring(path.lastIndexOf('/') + 1));

    // Check if originalFileName has extension
    var lastDotIdx = originalFileName.lastIndexOf('.');
    if (lastDotIdx != -1) {
      originalFileName = originalFileName.substring(0, lastDotIdx);
    }
    if (originalFileName.length > 50) {
      // truncate long path names.
      originalFileName = originalFileName.substring(0, 50);
    }
    if(opt_blockNum){
      originalFileName = [originalFileName,
                          '(inline_block#', opt_blockNum, ')'
                          ].join('');
    }
    return originalFileName;
  },

  /**
   * Returns a file's extensions for the given resource.
   * @param {string} fileName Full file name.
   * @return {string} Extension from the original URL.
   */
  getFileExtension: function(fileName) {
    // Check if fileName has extension
    var lastDotIdx = fileName.lastIndexOf('.');
    if (lastDotIdx != -1) {
      return fileName.substring(lastDotIdx + 1);
    }
    return '';
  },

  /**
   * Returns the contents of all scripts or styles including both inline and
   * external.
   *
   * @param {string} type Must be either 'script' or 'style'.
   * @return {Array.<Object>} An array of objects containing 'name' and
   *     'content'.
   */
  getContentsOfAllScriptsOrStyles: function(type) {
    var allTags = [];

    if ('script' != type && 'style' != type) {
      return allTags;
    }

    // Inline.
    var docUrls = PAGESPEED.Utils.getResources('doc', 'iframe');

    for (var i = 0, len = docUrls.length; i < len; ++i) {
      var sHTML = PAGESPEED.Utils.getResourceContent(docUrls[i]);
      var inline = PAGESPEED.Utils.getContentsOfAllTags(sHTML, type);
      var iNextInlineNumber = 1;
      for (var j = 0, jlen = inline.length; j < jlen; ++j) {
        if (inline[j]) {
          allTags.push({
            'name': [docUrls[i],
                     ' (inline block #', iNextInlineNumber, ')'].join(''),
            'content': inline[j],
            'url':docUrls[i],
            'blockNum':iNextInlineNumber++
          });
        }
      }
    }

    // External.
    var urls = PAGESPEED.Utils.getResources(type == 'script' ? 'js' : 'css');
    for (var i = 0, len = urls.length; i < len; ++i) {
      allTags.push({
        'name': urls[i],
        'content': PAGESPEED.Utils.getResourceContent(urls[i]),
        'url': urls[i],
        'blockNum': undefined
      });
    }

    return allTags;
  },

  /**
   * Returns the size of all resources for the current page. This ignores
   * compression and headers.
   */
  getTotalResourceSize: function() {
    var size = 0;
    var resources = PAGESPEED.Utils.getResources();
    for (var i = 0, len = resources.length; i < len; ++i) {
      size += PAGESPEED.Utils.getResourceSize(resources[i]);
    }
    return size;
  },

  /**
   * Returns the sum of the transfer size for all resources on the page.
   * The transfer size takes compression into account and includes the header
   * size.
   */
  getTotalTransferSize: function() {
    var size = 0;
    var resources = PAGESPEED.Utils.getResources();
    for (var i = 0, len = resources.length; i < len; ++i) {
      size += PAGESPEED.Utils.getResourceTransferSize(resources[i]);
    }
    return size;
  },

  /**
   * Returns the total number of requests made by the current page.
   */
  getTotalRequests: function() {
    return PAGESPEED.Utils.getResources().length;
  },

  /**
   * Returns the size of the given resource URL over the network. If the
   * resource is compressed, this will be the compressed size.
   *
   * @param {string} url The URL of the request.
   * @return {number} The resource's transfer size in bytes.
   */
  getResourceTransferSize: function(url) {
    function fetchTransferSize(url, out_shouldCache) {
      // First try the cache, since it's most accurate. We don't trust
      // the content length header, since serves sometimes put the
      // wrong content length in their responses.
      var cacheFile = PAGESPEED.Utils.getCacheEntry(url);
      if (cacheFile) {
        var size = cacheFile.dataSize;
        cacheFile.close();
        out_shouldCache.value = true;
        return size;
      }

      // Use the content length if available, but don't cache it, in
      // case the cache entry becomes available later this can happen
      // if the resource hasn't finished downloading, for
      // instance. Cache entry is only available after the resource
      // has been fully downloaded.
      var size = PAGESPEED.Utils.getResourceProperty(
          url, PAGESPEED.Utils.RESPONSE_CONTENT_LENGTH);
      if (!isNaN(size)) {
        return size;
      }

      // Finally, assume a zero transfer size if it's a redirect and
      // none of the other tests above have worked. But don't cache
      // this value, in case the cache entry becomes available later
      // and gives us a more accurate result.
      var responseCode = PAGESPEED.Utils.getResponseCode(url);
      if (responseCode == 301 || responseCode == 302) {
        // A redirect. Firefox 3.0.x does not store the response body
        // for redirects, so we return zero length if none of the
        // above tests found a transfer size.
      return 0;
      }

      return 0;
    }
    return Number(PAGESPEED.Utils.getAndCacheResourceProperty(
        url, PAGESPEED.Utils.TRANSFER_SIZE, fetchTransferSize));
  },

  /**
   * Sets a property for a resource.
   * @param {string} url The URL of the request.
   * @param {string} key The property name to set.
   * @param {*} value The property value to set.
   */
  setResourceProperty: function(url, key, value) {
    var components = PAGESPEED.Utils.getComponents();
    var types = PAGESPEED.Utils.getResourceTypes(url);
    for (var i = 0, len = types.length; i < len; i++) {
      components[types[i]][url][key] = value;
    }
  },

  /**
   * Gets a property for a resource.
   * @param {string} url The URL of the request.
   * @param {string} key The property name to get.
   * @return {*} The property value, or undefined if no property value
   *     could be found.
   */
  getResourceProperty: function(url, key) {
    var components = PAGESPEED.Utils.getComponents();
    var types = PAGESPEED.Utils.getResourceTypes(url);
    for (var i = 0, len = types.length; i < len; i++) {
      var component = components[types[i]][url];
      if (component && component[key]) {
        return component[key];
      }
    }
    return undefined;
  },

  /**
   * Check to see if the given property is cached. If so, return the
   * cached property. If not, invoke the fetch function, and if that
   * function indicates that it's ok to cache the result, cache it so
   * we don't have to look it up the next time around.
   * @param {string} url The URL to get a property for.
   * @param {string} propName The name of the cached property.
   * @param {Function} fetchFn The function responsible for fetching
   *     the value, if it's not cached already.
   * @return {Object} The fetched property.
   */
  getAndCacheResourceProperty: function(url, propName, fetchFn) {
    var property = PAGESPEED.Utils.getResourceProperty(url, propName);
    if (property) return property;
    var out_shouldCache = {};
    // If the callee sets out_shouldCache.value to true, then we cache
    // the returned property on the resource.
    property = fetchFn(url, out_shouldCache);
    if (out_shouldCache.value) {
      PAGESPEED.Utils.setResourceProperty(url, propName, property);
    }
    return property;
  },

  /**
   * Returns true if the given response headers indicate that the resource
   * is compressed. Currently recognizes gzip and deflate as valid compression
   * methods. Note that if sdch is used, it should be used in addition to gzip
   * or deflate.
   *
   * @param {Object} headers The HTTP Headers of the response.
   * @return {boolean} true iff the resource is compresed.
   */
  isCompressed: function(headers) {
    var contentEncoding = headers && headers['Content-Encoding'];
    if (!contentEncoding) return false;

    var encodings = contentEncoding.split(',');
    for (var i = 0, len = encodings.length; i < len; ++i) {
      var encoding = PAGESPEED.Utils.trim(encodings[i]);
      if (encoding == 'gzip' || encoding == 'deflate') {
        return true;
      }
    }
    return false;
  },

  /**
   * @param {number} code the response code.
   * @return {boolean} Whether a response with the given response code
   *     is cacheable in the absence of other caching headers.
   */
  isCacheableResponseCode: function(code) {
    switch (code) {
      // HTTP/1.1 RFC lists these response codes as cacheable in the
      // absence of explicit caching headers.
      case 200:
      case 203:
      case 206:
      case 300:
      case 301:
      case 410:
        return true;
      // In addition, 304s are sent for cacheable resources. Though
      // the 304 response itself is not cacheable, the underlying
      // resource is, and that's what we care about.
      case 304:
        return true;
      default:
        return false;
    }
  },

  /**
   * Returns the uncompressed size of the given resource URL.
   * @param {string} url The URL of the request.
   * @return {number} The resource's size in bytes.
   */
  getResourceSize: function(url) {
    function fetchResourceSize(url, out_shouldCache) {
      var storageStream = PAGESPEED.Utils.getResourceProperty(
          url, PAGESPEED.Utils.RESPONSE_STORAGE_STREAM);
      if (storageStream) {
        // Only cache the value if the storage stream is no longer
        // being written to.
        out_shouldCache.value = !storageStream.writeInProgress;
        return storageStream.length;
      }

      var inputStream = PAGESPEED.Utils.getResourceInputStream(url);
      if (inputStream != null) {
        var numBytes = PAGESPEED.Utils.countBytesInStream(inputStream);
        inputStream.close();
        out_shouldCache.value = (numBytes > 0);
        return numBytes;
      }

      var responseCode = PAGESPEED.Utils.getResponseCode(url);
      if (responseCode == 301 || responseCode == 302) {
        // A redirect. Firefox 3.0.x does not store the response body
        // for redirects, so we return zero length if none of the
        // above tests found a transfer size.
        return 0;
      }

      return 0;
    }
    return Number(PAGESPEED.Utils.getAndCacheResourceProperty(
        url, PAGESPEED.Utils.RESOURCE_SIZE, fetchResourceSize));
  },

  /**
   * Reads contents from an inputStream into a string. Closes the input stream
   * when finished.
   * @param {Object} inputStream The input stream to read.
   * @return {string?} The string representation of the input stream,
   *     or null if the stream could not be read.
   */
  readInputStream: function(inputStream) {
    if (!inputStream) {
      return null;
    }
    var scriptableStream =
      PAGESPEED.Utils.CCIN(
          '@mozilla.org/scriptableinputstream;1',
          'nsIScriptableInputStream');
    scriptableStream.init(inputStream);
    var data = [];
    while (scriptableStream.available() > 0) {
      var str = scriptableStream.read(scriptableStream.available());
      data.push(str);

      // If the stream contains binary data, it will get truncated
      // before we read all the data. In this case, make sure we bail.
      if (!str.length) break;
    }
    // Wrap close() calls in try/catch. It is non-fatal if close()
    // fails, so we just silently discard the exception.
    PAGESPEED.Utils.tryToCloseStream(scriptableStream);
    PAGESPEED.Utils.tryToCloseStream(inputStream);
    return data.join('');
  },

  /**
   * Retrieve the collection of components for the window.
   * @return {Object} The components of the window.
   */
  getComponents: function() {
    var components;
    var win = Firebug.currentContext.window;
    if (win) {
      components = PAGESPEED.Utils.getComponentCollector()
          .getWindowComponents(win);
    }

    if (components) {
      return components;
    } else {
      PS_LOG('Could not retrieve components from window.');
      return {};
    }
  },

  /**
   * Return a list of elements matching the indicated type.
   * @param {string} type The type of the elements to match.
   * @return {Array.<Object>} A list of matching elements.
   */
  getElementsByType: function(type) {
    var components = PAGESPEED.Utils.getComponents();
    var elements = [];
    if (components[type] && typeof(components[type]) == 'object') {
      for (var n in components[type]) {
        elements = elements.concat(components[type][n].elements);
      }
    }
    return elements;
  },

  /**
   * Return a list of unique resource URLs.
   *
   * Pass this function a list of component type names from
   * components/ComponentCollector.js.
   *
   * If no arguments are given, all types are returned.
   *
   * @return {Array.<string>} A list of URLs.
   */
  getResources: function(var_args) {
    return PAGESPEED.Utils.getResourcesWithFilter(
        PAGESPEED.Utils.alwaysReturnTrueFn,
        arguments
        );
  },

  /**
   * Return a list of unique resource URLs.
   * @param {Function} filterFn This function will be run on each url record.
   *     Only urls that cause it to return true are kept.  If unset, allow
   *     all urls.
   * @param {Array.<string>} opt_types Return urls with these resource types.
   *     If unset or empty, all types are returned.
   *
   * @return {Array.<string>} A list of URLs.
   */
  getResourcesWithFilter: function(filterFn, opt_types) {
    var components = PAGESPEED.Utils.getComponents();

    // Copy all of the arguments to the target types. If none were given, copy
    // all types.
    var types;
    if (!opt_types || !opt_types.length) {
      types = [];
      // Get all types.
      for (var type in components) {
        if (!components[type] || typeof(components[type]) != 'object')
          continue;
        types.push(type);
      }
    } else {
      types = opt_types;
    }

    var uniqueResources = {};
    for (var i = 0, len = types.length; i < len; i++) {
      if (!components[types[i]] || typeof(components[types[i]]) != 'object')
        continue;
      for (var url in components[types[i]]) {
        var requestTime = components[types[i]][url].requestTime;
        if (!filterFn(url, components[types[i]][url])) {
          continue;
        }
        uniqueResources[url] = 1;
      }
    }

    var resources = [];
    for (var n in uniqueResources) {
      resources.push(n);
    }

    return resources;
  },

  /**
   * Strip URI fragment, i.e. anything after the #.
   * @param {string} uri A uri.
   * @return {string} |uri| without the #fragment.
   */
  stripUriFragment: function(uri) {
    return uri.replace(/#.*$/, '');
  },

  /**
   * The component collector saves redirects as an adjacency list which holds
   * the redirect DAG.  For example, suppose we have the following graph:
   *   a1.com -> b.com
   *   a2.com -> b.com
   *   b.com -> c.com
   * The redirect object will look like this:
   *
   * PAGESPEED.Utils.getComponents().redirect = {
   *   "http://a1.com/":{
   *     "elements":["http://b.com/"],
   *     lots of other info
   *   },
   *   "http://a2.com/":{
   *     "elements":["http://b.com/"],
   *     lots of other info
   *   },
   *   "http://b.com/":{
   *     "elements":["http://c.com/"],
   *     lots of other info
   *   },
   * }
   * If a user types http://a1.com into firefox, we want to mark results as
   * coming from a1.com, even though the browser was at c.com when the
   * scoring was done.
   *
   * It is imposible to truly know what the user entered, but following the dag
   * to a node with no incoming edge is going to be right most of the time.
   *
   * @param {string} url A url scored by page speed.
   * @return {string} The shortest url that we redirected from to get to |url|.
   */
  findPreRedirectUrl: function(url) {
    url = PAGESPEED.Utils.stripUriFragment(url);

    var redirects = PAGESPEED.Utils.getComponents().redirect;

    // If we can't get the table of redirects, than there were no redirects.
    // Return the initial url.
    if (!redirects)
      return url;

    // We have a map from the url that redirects (source) to the url
    // redirected to (dest).  We need to walk the graph from dest to
    // source, so invert the graph.
    var destToSource = {};
    for (var fromUrl in redirects) {
      var elements = redirects[fromUrl].elements;
      for (var i = 0, len = elements.length; i < len; i++) {
        var toUrl = elements[i];
        if (!destToSource[toUrl]) {
          destToSource[toUrl] = [];
        }
        destToSource[toUrl].push(fromUrl);
      }
    }

    // Now find all urls reachable from |url|.
    var reachableUrlSet = {};
    var sourceUrls = [];  // Hold urls that are not destinations of any source.

    var toAddToSet = [url];  // Hold the urls not yet traversed.

    while (toAddToSet.length) {
      var curUrl = toAddToSet.pop();

      // Never visit a url more than once.
      if (reachableUrlSet[curUrl]) continue;
      reachableUrlSet[curUrl] = true;

      // If |curUrl| is not redirected to from anouther url,
      // than it is a good candidate for the initial url.
      if (!destToSource[curUrl]) {
        sourceUrls.push(curUrl);
        continue;
      }

      // Add any urls that redirect to |curUrl| into the set of urls
      // to test.  Duplicates are okay.  They are filtered by |reachableUrlSet|.
      toAddToSet = toAddToSet.concat(destToSource[curUrl]);
    }

    if (!sourceUrls.length) {
      // There are no URLs in the redirect chain that were not redirected to
      // from some other URL.  This means there is a cycle in the graph of
      // redirects, which is very unusual.  Just return the original url.
      return url;
    }

    // Typical there will be only one source URL.
    // To make sure this function is deterministic, return the shortest url,
    // with alphabet order used to break ties.

    var shortestFromUrl = sourceUrls[0];

    for (var i = 1, len = sourceUrls.length; i < len; ++i) {
      var sourceUrl = sourceUrls[i];

      if (shortestFromUrl.length < sourceUrl.length)
        continue;

      if (shortestFromUrl.length == sourceUrl.length &&
          shortestFromUrl < sourceUrl)
        continue;

      shortestFromUrl = sourceUrl;
    }

    return shortestFromUrl;
  },

  /**
   * This function always returns true.  Useful as a callback.
   * @return {boolean} true.
   */
  alwaysReturnTrueFn: function(var_args) {
    return true;
  },

  /**
   * Retrieve the cache entry for a given URL.
   * @param {string} url The URL of the resource to fetch from cache.
   * @return {nsICacheEntryDescriptor} The cache descriptor for the resource or
   *   undefined if there is no cache entry for the url.
   */
  getCacheEntry: function(url) {
    var file;
    var service = PAGESPEED.Utils.CCSV(
        '@mozilla.org/network/cache-service;1', 'nsICacheService');
    if (!service) {
      return null;
    }

    url = url.replace(reUrlFragment, '');  // Trim fragment.
    var nsICache = Components.interfaces.nsICache;
    var session = service.createSession('HTTP', nsICache.STORE_ANYWHERE,
                                        nsICache.STREAM_BASED);
    session.doomEntriesIfExpired = false;
    try {
      file = session.openCacheEntry(url, nsICache.ACCESS_READ,
                                    nsICache.NON_BLOCKING);
    } catch (e) {
      // There are two exceptions which session.openCacheEntry()
      // might throw:
      //
      // NS_ERROR_CACHE_KEY_NOT_FOUND (0x804B003D): Thrown when the url
      // contents are not in the cache.
      //
      // NS_ERROR_CACHE_WAIT_FOR_VALIDATION (0x804B0040): Thrown when
      // the url contents are in the cache, but not yet marked valid.
      // Calling openCacheEntry(..., nsICache.BLOCKING) would block
      // until the entry gets marked valid.  This is a problem for
      // pages that fetch a url that never seems to finish downloading
      // (e.g. for a server-initiated RPC)
      // Therefore, openCacheEntry() will block forever.

      // To see which exception was thrown, test e.result .
    }

    return file;
  },

  /**
   * Fetch the current document URL or return an empty string.
   * @return {string} The URL of the primary doc component.
   */
  getDocumentUrl: function() {
    var aDocUrls = PAGESPEED.Utils.getResources('doc');
    if (aDocUrls && aDocUrls[0]) {
      if (aDocUrls.length > 1) {
        PS_LOG('ERROR: Found ' + aDocUrls.length + ' documents');
      }
      return aDocUrls[0];
    }
    PS_LOG('ERROR finding document');
    return '';
  },

  /**
   * Fetch the current document or return an empty string.
   * @return {string} The source of the primary doc component.
   */
  getDocumentContent: function() {
    var url = PAGESPEED.Utils.getDocumentUrl();
    return url ? PAGESPEED.Utils.getResourceContent(url) : '';
  },

  /** Returns true iff the given depency is statisfied. */
  dependencyIsSatisfied: function(dependency) {
    if (!dependency.installedVersion) {
      return false;
    }

    /**
     * Fixes common mistakes in extension version numbers that don't conform to
     * Mozilla's versioning scheme.
     */
    var fixInvalidVersion = function(version) {
      // Firebug incorrectly uses "1.05" like "1.0.5". Mozilla interprets "05"
      // as "5", so we have to correct that here.
      return version.replace(reInvalidVersion, '0.$1');
    };

    var vc = PAGESPEED.Utils.CCSV(
        '@mozilla.org/xpcom/version-comparator;1', 'nsIVersionComparator');

    return (vc.compare(fixInvalidVersion(dependency.installedVersion),
                       fixInvalidVersion(dependency.minimumVersion)) >= 0 &&
            vc.compare(fixInvalidVersion(dependency.installedVersion),
                       fixInvalidVersion(dependency.maximumVersion)) <= 0);
  },

  /**
   * Retrieves the version of an installed extension
   */
  getDependencyVersion: function(extensionName, callback) {
    try {
      var em = PAGESPEED.Utils.CCSV(
          '@mozilla.org/extensions/manager;1', 'nsIExtensionManager');
      if (em) {
        var addon = em.getItemForID(extensionName);
        if (addon && addon.version) {
          callback(addon.version);
        } else {
          callback(null);
        }
      } else {
        AddonManager.getAddonByID(extensionName, function(addon) {
            if (addon && addon.version) {
              callback(addon.version);
            } else {
              callback(null);
            }
          });
      }
    } catch (e) {
      PS_LOG('getDependencyVersion: ' + e);
    }
  },

  /**
   * Fetch the version of Page Speed from the add-on manager.
   */
  fetchPageSpeedVersion: function() {
    PAGESPEED.Utils.getDependencyVersion(PAGESPEED_GUID_, function(version) {
      PAGESPEED.PAGESPEED_VERSION = version;
    });
  },

  /**
   * Retrieve the version of Page Speed.
   */
  getPageSpeedVersion: function() {
    if (!PAGESPEED.PAGESPEED_VERSION) {
      PS_LOG('No Page Speed version available.');
    }
    return PAGESPEED.PAGESPEED_VERSION;
  },

  /**
   * Retrieve the versions of all dependencies.
   */
  updateDependencyVersions: function(callback) {
    var counter = 0;
    for (var addonName in PAGESPEED.DEPENDENCIES) {
      if (!PAGESPEED.DEPENDENCIES.hasOwnProperty(addonName)) continue;
      var version = PAGESPEED.Utils.getDependencyVersion(
          PAGESPEED.DEPENDENCIES[addonName].installedName, function(version) {
            PAGESPEED.DEPENDENCIES[addonName]['installedVersion'] = version;
            counter++;
            if (counter == PAGESPEED.NUM_DEPENDENCIES) {
              callback();
            }
          });
    }
  },

  /**
   * Check that the namespaces of required extensions are available.
   */
  checkNamespaceDependencies: function() {
    for (var addonName in PAGESPEED.DEPENDENCIES) {
      if (PAGESPEED.DEPENDENCIES[addonName].hasOwnProperty('namespace') &&
          !window[PAGESPEED.DEPENDENCIES[addonName].namespace]) {
        PAGESPEED.isHealthy = false;
        alert(['Page Speed depends on the ', addonName, ' extension, which is ',
               'disabled or not installed. Please enable or install ',
               addonName, ' to begin using Page Speed.'].join(''));
      }
    }
  },

  /**
   * Get the version of firefox we are using.  Note that this might not match
   * the version in the user agent because the user agent can be faked.
   * @return {string} The firefox version we are running in.
   */
  getFirefoxVersion: function() {
    // See https://developer.mozilla.org/en/Using_nsIXULAppInfo
    // for docs on this service.
    var appInfo = PAGESPEED.Utils.CCSV('@mozilla.org/xre/app-info;1',
                                       'nsIXULAppInfo');
    // TODO: Would it be usefull to check that we are runing in firefox?
    // See the link above for docs on how to do this.

    return appInfo.version;
  },

  /**
   * Filter a list of component URLs by applying the given regexp to each
   * URL and returning only those that match.
   *
   * @param {Array.<string>} urls An array of component URLs to filter.
   * @param {RegExp} rExpr The RegExp to filter by.
   * @param {boolean?} opt_bNegate Iff true, returns non matches
   *     instead of matches.
   * @return {Array.<string>} The filtered components.
   */
  filterByRegexp: function(urls, rExpr, opt_bNegate) {
    var tempComponents = [];
    for (var i = 0; i < urls.length; i++) {
      var matched = !!urls[i].match(rExpr);
      if (opt_bNegate ? !matched : matched) {
        tempComponents.push(urls[i]);
      }
    }
    return tempComponents;
  },

  /**
   * Filter a list of Components by protocol, keeping only Components
   * retrieved via http and https.
   *
   * @param {Array.<string>} urls A list of component URLs to filter.
   * @return {Array.<string>} The filtered components.
   */
  filterProtocols: function(urls) {
    return PAGESPEED.Utils.filterByRegexp(urls, reHttp, false);
  },

  /**
   * Get the currently active context for the pagespeed panel.
   */
  getPageSpeedContext: function() {
    return Firebug.currentContext &&
      Firebug.currentContext.getPanel('pagespeed') &&
      Firebug.currentContext.getPanel('pagespeed').context;
  },

  /**
   * Get the browser preferences object.
   */
  getPrefs: function() {
    return PAGESPEED.Utils.CCSV(
        '@mozilla.org/preferences-service;1', 'nsIPrefBranch');
  },

  /**
   * Reset a preference to the default value.
   * @param {String} prefName The name of the pref to clear.
   */
  clearPref: function(prefName) {
    var prefs = PAGESPEED.Utils.getPrefs();
    try {
      prefs.clearUserPref(prefName);
    } catch (e) {
      // clearUserPref throws an exception if there is nothing to clear.
      // Do nothing in this case.
    }
  },

  /**
   * Check if a boolean preference is set.  If so, return its value.
   * If not, return the default value passed as an argument.
   * @param {string} prefName The name of the preference to fetch.
   * @param {boolean} opt_defaultValue The default value to use if the
   *     pref is undefined or not a boolean.
   * @return {boolean} The preference value.
   */
  getBoolPref: function(prefName, opt_defaultValue) {
    var prefs = PAGESPEED.Utils.getPrefs();
    if (prefs.getPrefType(prefName) == prefs.PREF_BOOL) {
      return prefs.getBoolPref(prefName);
    } else {
      return opt_defaultValue || false;
    }
  },

  /**
   * Check if an integer preference is set.  If so, return its value.
   * If not, return the default value passed as an argument.
   * @param {string} prefName The name of the preference to fetch.
   * @param {number} defaultValue The default value to use if the
   *     pref is undefined or not a number.
   * @return {number} The preference value.
   */
  getIntPref: function(prefName, defaultValue) {
    var prefs = PAGESPEED.Utils.getPrefs();
    if (prefs.getPrefType(prefName) == prefs.PREF_INT) {
      return prefs.getIntPref(prefName);
    } else {
      return defaultValue;
    }
  },

  /**
   * Check if a string preference is set.  If so, return its value.
   * If not, return null.
   * @param {string} prefName The name of the preference to fetch.
   * @return {string?} The preference value, or null.
   */
  getStringPref: function(prefName) {
    var prefs = PAGESPEED.Utils.getPrefs();
    if (prefs.getPrefType(prefName) == prefs.PREF_STRING) {
      return prefs.getCharPref(prefName);
    } else {
      return null;
    }
  },

  /**
   * Get the value of a preference.  The preference can be a string,
   * int, or bool.  Return undefined if there is no such preference.
   * @param {string} prefName The name of the preference to fetch.
   * @return {string|number|boolean|undefined} The preference value.  undefined
   *    if there is no such preference.
   */
  getPrefValue: function(prefName) {
    var prefs = PAGESPEED.Utils.getPrefs();
    var result;
    switch (prefs.getPrefType(prefName)) {
      case prefs.PREF_STRING:
        result = prefs.getCharPref(prefName);
        break;
      case prefs.PREF_INT:
        result = prefs.getIntPref(prefName);
        break;
      case prefs.PREF_BOOL:
        result = prefs.getBoolPref(prefName);
        break;
      default:
        result = undefined;
    }
    return result;
  },

  /**
   * Set a boolean preference.  Create the pref if necessary, and overwrite
   * an existing pref if necessary.
   * @param {string} prefName The name of the preference to set.
   * @param {boolean|undefined|null} value The value to set the pref to.
   */
  setBoolPref: function(prefName, value) {
    PAGESPEED.Utils.getPrefs().setBoolPref(prefName, value);
  },

  /**
   * Set a preference to the path of a file.
   * @param {string} prefName The name of the preference to set.
   * @param {nsIFile} file The file whose path is to be stored.
   */
  setFilePref: function(prefName, file) {
    var pageSpeedPrefs = PAGESPEED.Utils.getPrefs();
    pageSpeedPrefs.setComplexValue(prefName,
                                   Components.interfaces.nsILocalFile,
                                   file);
  },

  /**
   * Read a preference and construct a file object that represents it.
   * @param {string} prefName The name of the preference to set.
   * @return {nsIFile} The file named by the preference, or null if the
   *     preference is unset.
   */
  getFilePref: function(prefName) {
    var pageSpeedPrefs = PAGESPEED.Utils.getPrefs();
    try {
      return pageSpeedPrefs.getComplexValue(prefName,
					    Components.interfaces.nsILocalFile);
    } catch (e) {
      // If the pref is unset, an exception will be thrown.
      return null;
    }
  },

  /**
   * @param {string} name The name of the cache object to fetch.
   * @return {Object} A singleton object stored in the IStateStorage
   *     instance with the given string name. Multiple calls to this
   *     function with the same name parameter are guaranteed to
   *     return the same object, even if called from different browser
   *     windows.
   */
  getCachedObject: function(name) {
    // We use an xpcom service object to cache objects, since xpcom
    // services are singletons, even across browser windows.
    var pagespeedService = PAGESPEED.Utils.CCSV(
        '@code.google.com/p/page-speed/StateStorageService;1', 'IStateStorage');

    // wrappedJSObject lets us get at the underlying javascript object
    // wrapped by the pagespeedService xpconnect interface. We must use
    // the wrappedJSObject because getCachedObject returns a JS
    // object, and it's not possible to return a JS object via
    // xpconnect.
    return pagespeedService.wrappedJSObject.getCachedObject(name);
  },

  /**
   * @return {IComponentCollector} the IComponentCollector singleton
   *     instance.
   */
  getComponentCollector: function() {
    return PAGESPEED.Utils.CCSV(
        '@code.google.com/p/page-speed/ComponentCollectorService;1',
        'IComponentCollector').wrappedJSObject;
  },

  /**
   * Given the source code of a script from the Firefox JavaScript
   * debugger, we clean it up to make it more presentable to the user.
   */
  cleanUpJsdScriptSource: function(src) {
    // For now, all we do is trim 4 spaces from the beginning of each
    // line.
    return src.replace(reFourSpaces, '');
  },

  /**
   * Remove the query string and/or the hash from the URL.
   */
  getBaseUrl: function(url) {
    return url.replace(reUrlFragmentOrQuerystring, '');
  },

  /**
   * Copy properties which are basic types from one object on to another.
   * If both objects have a property with the same name, the property in
   * the object being copied from is used.
   * @param {Object} dest The object to copy properties to.
   * @param {Object} source The object to copy properties from.
   * @param {string} opt_prefix A prefix to add to each destination property.
   */
  copyAllPrimitiveProperties: function(dest, source, opt_prefix) {
    var prefix = opt_prefix || '';
    for (var key in source) {
      switch (typeof(source[key])) {
        case 'string':
        case 'number':
        case 'boolean':
        case 'undefined':
          dest[prefix + key] = source[key];
          break;
        case 'object':  // Only copy the null object.
          if (source[key] === null)
            dest[prefix + key] = null;
          break;
        default:
          // Do not copy non-null objects or functions.
      }
    }
  },

  /**
   * A regex for identifying excess whitespaces in HTML
   */
  HTML_WHITESPACE: /^([\n\t ]+)|([\n\t ]{2,})$|[\n\t ]([\n\t ]+)/gm,

  /**
   * Returns a list of all of the matches for a regular expression.
   * @param {RegExp} re The regular expression to match on.
   * @param {string} matchString The string to match against.
   * @param {Array.<number>} indexes The indexes of the match groups to append
   *     to the result.
   * @return {Array.<string>} The array of matches.
   */
  scanningRegexMatch: function(re, matchString, indexes) {
    var aResults = [];
    var aMatches;
    while ((aMatches = re.exec(matchString)) != null) {
      for (var i = 0; i < indexes.length; i++) {
        if (aMatches[indexes[i]] != undefined) {
          aResults.push(aMatches[indexes[i]]);
        }
      }
    }
    return aResults;
  },

  /**
   * @param {string} dataString The desired content of the input
   *     stream.
   * @return {nsIInputStream} An input stream.
   */
  wrapWithInputStream: function(dataString) {
    var stringStream = PAGESPEED.Utils.CCIN(
        '@mozilla.org/io/string-input-stream;1', 'nsIStringInputStream');
    if ('data' in stringStream) // Gecko 1.9 or newer
      stringStream.data = dataString;
    else // 1.8 or older
      stringStream.setData(dataString, dataString.length);
    return stringStream;
  },

  /**
   * Builds a mozilla.org/network/mime-input-stream from a string.
   *
   * POST method requests must wrap the encoded text in a MIME stream.
   * The constructed stream can be used as the postData argument for to an
   * addTab() call, for example.
   *
   * This code is taken from
   * http://developer.mozilla.org/en/docs/Code_snippets:Post_data_to_window
   *
   * @param {string} dataString The desired content of the mime input stream.
   * @return {Object} A mime input stream.
   */
  createPostData: function(dataString) {
    var stringStream = PAGESPEED.Utils.wrapWithInputStream(dataString);

    var postData = PAGESPEED.Utils.CCIN(
        '@mozilla.org/network/mime-input-stream;1', 'nsIMIMEInputStream');
    postData.addHeader('Content-Type', 'application/x-www-form-urlencoded');
    postData.addContentLength = true;
    postData.setData(stringStream);

    return postData;
  },

  /**
   * Calculates the size of the given function source.
   *
   * This function will subtract the size of any nested functions.
   *
   * @param {string} text The function source code.
   * @return {number} The length of the function source code.
   */
  getFunctionSize: function(text) {
    var functionSize = text.length;
    var functionStr = 'function';
    var functionIndex = text.indexOf(functionStr);
    var index = functionIndex + functionStr.length;
    while (index < text.length) {
      var nestedFunctionLength =
          PAGESPEED.Utils.findNestedFunctionLength(text, index);
      if (nestedFunctionLength > 0) {
        functionSize -= nestedFunctionLength;
        index += nestedFunctionLength;
        continue;
      }
      var skipLength = (
          // Skip over literals that may contain "function".
          PAGESPEED.Utils.findStringLiteralLength(text, index) ||
          PAGESPEED.Utils.findCommentLength(text, index) ||
          PAGESPEED.Utils.findRegexLength(text, index));
      if (skipLength > 0) {
        index += skipLength;
      } else {
        index++;
      }
    }
    return functionSize;
  },

  /**
   * Regex to find out if a function definition is there. This needs to start
   * one character back to avoid things like 'my_function'.
   */
  FUNCTION_REGEX: /^\Wfunction\W$/,

  /**
   * Finds the length of this nested function.
   *
   * This looks behind one character to avoid matching names like 'my_function'.
   * If the end is not found, it includes everything to the end of the string.
   *
   * @param {string} text The javascript source code.
   * @param {number} index The start of a possible match.
   * @return {number} The function length including 'function' and '}'.
   */
  findNestedFunctionLength: function(text, index) {
    var endIndex = index;
    if (text[index] == 'f') {
      var functionStr = 'function';
      var functionStart = text.substr(index - 1, functionStr.length + 2);
      if (functionStart.match(PAGESPEED.Utils.FUNCTION_REGEX)) {
        var numParens = 0;
        endIndex += functionStart.length;
        while (endIndex < text.length) {
          var skipLength = (
              // Skip over literals that may contain curly braces.
              PAGESPEED.Utils.findStringLiteralLength(text, endIndex) ||
              PAGESPEED.Utils.findCommentLength(text, endIndex) ||
              PAGESPEED.Utils.findRegexLength(text, endIndex));
          if (skipLength > 0) {
            endIndex += skipLength;
            continue;
          }
          if (text[endIndex] == '{') {
            numParens++;
          } else if (text[endIndex] == '}') {
            numParens--;
            if (numParens == 0) {
              endIndex++;  // count the closing '}'
              break;
            }
          }
          endIndex++;
        }
      }
    }
    return endIndex - index;
  },

  /** Finds the length of a string literal. Returns zero on no match.
   *
   * @param {string} text The javascript source code.
   * @param {number} index The start of a possible match.
   * @return {number} The length a string literal including the quotes.
   */
  findStringLiteralLength: function(text, index) {
    var endIndex = index;
    if (text[index] == '\'' || text[index] == '"') {
      var quoteChar = text[endIndex++];
      while (endIndex < text.length) {
        if (text[endIndex] == quoteChar && text[endIndex - 1] != '\\') {
          endIndex++;  // count the final quote character
          break;
        }
        endIndex++;
      }
    }
    return endIndex - index;
  },

  /**
   * Finds the length of this comment.
   *
   * For line comments (//), it includes the '\n' at the end of the line.
   * For block comments it includes the start and ending slashes.
   * If the end is not found, it includes everything to the end of the string.
   *
   * @param {string} text The javascript source code.
   * @param {number} index The index of the start of the comment.
   *                        This can either be a block comment or a
   *                        single-line comment.
   * @return {number} The length of the comment.
   */
  findCommentLength: function(text, index) {
    var endIndex = 0;
    if (text.substr(index, 2) == '/*') {
      endIndex = text.indexOf('*/', index + 2) + 1;  // index of last slash
    } else if (text.substr(index, 2) == '//') {
      endIndex = text.indexOf('\n', index + 2);      // index of newline
    } else {
      return 0;  // no comment found
    }
    if (endIndex <= 0) {
      endIndex = text.length - 1;  // comment runs off the end
    }
    return endIndex - index + 1;
  },

  /**
   * Finds the length of a regex. Returns zero on no match.
   *
   * @param {string} text The javascript source code.
   * @param {number} index The start of a possible match.
   * @return {number} The length of a regex including the slashes.
   */
  findRegexLength: function(text, index) {
    var endIndex = index;
    if (text[index] == '/' && text[index + 1] != '/') {
      endIndex++;
      while (endIndex < text.length) {
        if (text[endIndex] == '/' && text[endIndex - 1] != '\\') {
          endIndex++;  // count the final slash character
          break;
        }
        endIndex++;
      }
    }
    return endIndex - index;
  },

  /**
   * @return {nsIIOService} The IO service singleton.
   */
  getIOService: function() {
    return PAGESPEED.Utils.CCSV(
        '@mozilla.org/network/io-service;1', 'nsIIOService');
  },

  /**
   * @param {string} url The URL to fetch a channel for.
   * @return {nsIChannel} The newly created channel.
   */
  getChannelFromIOService: function(url) {
    var ioService = PAGESPEED.Utils.getIOService();
    return ioService.newChannelFromURI(
        ioService.newURI(url, null, null));
  },

  /**
   * Open a file.
   * @param {string} fileName The file name to write.
   * @return {nsIFile} The file.  null on error.
   */
  openFile: function(fileName) {
    if (fileName.length > MAX_FILEPATH_LENGTH) {
      PS_LOG("Error: filename too long in openFile.  " +
             fileName);
      return null;
    }

    var localFile = PAGESPEED.Utils.CCIN(
        '@mozilla.org/file/local;1', 'nsILocalFile');
    localFile.initWithPath(fileName);
    return localFile;
  },

  /**
   * If no output directory is set, then set to the equivalent of /tmp
   * on the OS we are running under.  Setting a default in preferences.js
   * is not possible because different OSes need different values.
   */
  setDefaultOutputDir: function() {
    // Do nothing if the pref exists and has a non-empty value.
    if (PAGESPEED.Utils.getFilePref(OPTIMIZED_FILE_BASE_DIR))
      return;

    // Get the system temp directory.
    var tmpDir = PAGESPEED.Utils.CCSV(
        '@mozilla.org/file/directory_service;1', 'nsIProperties')
        .get('TmpD', Components.interfaces.nsIFile);

    PAGESPEED.Utils.setFilePref(OPTIMIZED_FILE_BASE_DIR, tmpDir);
  },

  /**
   * @param {nsIFile} outDir Set the output directory to this directory.
   */
  setOutputDir: function(outDir) {
    PAGESPEED.Utils.setFilePref(OPTIMIZED_FILE_BASE_DIR, outDir);
  },

  /**
   * Get a directory where rules can write files.
   * @param {string} opt_subDir Return this sub-directory within the scratch dir.
   * @return {nsIFile} The user's home dir for the current client.  null
   *     on error.
   */
  getOutputDir: function(opt_subDir) {
    var PERMISSIONS = 0700;

    try {
      var outputDir = PAGESPEED.Utils.getFilePref(OPTIMIZED_FILE_BASE_DIR);
      if (!outputDir) {
        // The first time this function is called, the preference might not
        // be set.  Set a default value.
        PAGESPEED.Utils.setDefaultOutputDir();
        outputDir = PAGESPEED.Utils.getFilePref(OPTIMIZED_FILE_BASE_DIR);
      }

      // A failure after the default is set must be a bug.  Log and continue.
      if (!outputDir) {
        PS_LOG('Failed to get output dir.');
        return null;
      }

      if (opt_subDir) {
        outputDir.append(opt_subDir);
      }

      if (outputDir.exists() && !outputDir.isDirectory()) {
        PS_LOG('Failed to create output directory "' + outputDir.path +
               '": a file already exists at that path.');
        return null;
      }

      if (!outputDir.exists()) {
        // create() will create intermediate dirs.  For example,
        // creating /none/of/these/dirs/exist/ will create five
        // directories.  The only time we should set permissions
        // on a directory is when we create that directory.
        outputDir.create(outputDir.DIRECTORY_TYPE, PERMISSIONS);

        // When create() fails to set permissioons as we ask, setting
        // permissions with outputDir.permissions = PERMISSIONS fails
        // in the same way, but throws an exception.  We log the
        // permisions are logged if they are not what we expect,
        // so that users who see problems can report issues including
        // the permissions they see.
        if (PERMISSIONS != outputDir.permissions) {
          var permToOctal = function(perms) {
            return Number(perms).toString(8);
          };

          PS_LOG(['getOutputDir(): Tried to set output directory ',
                  'permissions to ', permToOctal(PERMISSIONS), '.  ',
                  'Permissions are ', permToOctal(outputDir.permissions),
                  '.'].join(''));
        }
      }
      if (!outputDir.isDirectory() || !outputDir.isWritable()) {
        PS_LOG('getOutputDir(): Directory ' + outputDir.path +
               ' is not writable.');
        return null;
      }

      return outputDir.clone();

    } catch (e) {
      PS_LOG('getOutputDir(): Failed to create output directory "' +
             outputDir.path + '" due to an exception: ' +
             PAGESPEED.Utils.formatException(e) + ')');
      return null;
    }
  },

  /**
   * @param {nsIFile} f The file to generate a URL for.
   * @return {string} The URL for the given file (e.g. file:///a/b.jpg).
   */
  getUrlForFile: function(f) {
    return PAGESPEED.Utils.getIOService().newFileURI(f).spec;
  },

  /**
   * @param {nsIFile} f The file to generate a path to.
   * @return {string} The path for the given file (e.g. /a/b.jpg).
   */
  getPathForFile: function(f) {
    var url = PAGESPEED.Utils.getUrlForFile(f);

    // Strip off the 'file://', if present.
    var path = url.replace(/^file:\/\//, '');

    return path;
  },

  /**
   * Open a file for writing.
   * @param {string} fileName The file name to write.
   * @return {nsIOutputStream} The file output stream.  null on error.
   */
  openFileForWriting: function(fileName) {
    try {
      if (fileName.length > MAX_FILEPATH_LENGTH) {
        PS_LOG("Error: filename too long in openFileForWriting.  " +
               fileName);
        return null;
      }

      var localFile = PAGESPEED.Utils.openFile(fileName);

      var writeFlag = 0x02; // write only
      var createFlag = 0x08; // create
      var truncateFlag = 0x20; // truncate

      var fileOutputStream = PAGESPEED.Utils.CCIN(
          '@mozilla.org/network/file-output-stream;1', 'nsIFileOutputStream');

      fileOutputStream.init(localFile,
                            writeFlag | createFlag | truncateFlag,
                            0664, null);
    } catch (e) {
      PS_LOG("Exception in openFileForWriting: " +
             PAGESPEED.Utils.formatException(e));
      return null;
    }
    return fileOutputStream;
  },

  /**
   * Attempt to close the given stream. If an exception is thrown, we
   * silently ignore that exception. In some cases, if there's been an
   * error during stream read/write, closing that stream will
   * throw. We want to always attempt to close a stream, so we just
   * wrap the close call in a try/catch to handle these error cases as
   * well.
   * @param {nsIInputStream|nsIOutputStream} stream The stream to
   *     attempt to close.
   */
  tryToCloseStream: function(stream) {
    try {
      stream.close();
    } catch (e) {
    }
  },

  /**
   * @param {nsIInputStream} input The stream to copy from.
   * @param {nsIOutputStream} output The stream to copy to.
   * @param {number} numBytes The number of bytes to copy.
   * @param {boolean?} opt_dontClose If true, don't close the
   *     streams.
   * @return {number} The number of bytes written.
   */
  copyInputToOutput: function(input, output, numBytes, opt_dontClose) {
    var bufferedOut = PAGESPEED.Utils.CCIN(
        '@mozilla.org/network/buffered-output-stream;1',
        'nsIBufferedOutputStream');
    bufferedOut.init(output, 4096);
    var numBytesWritten = 0;
    try {
      numBytesWritten = bufferedOut.writeFrom(input, numBytes);
      bufferedOut.flush();
    } catch (e) {
      PS_LOG('Failed to copy input to output ' + e);
    } finally {
      if (!opt_dontClose) {
        // Attempt to close both streams. Silently catch and ignore any
        // errors. If writeFrom() threw above, then closing the streams
        // may also throw (depending on the reason for the original
        // throw) so we have to wrap these closes in try/catches as
        // well.
        PAGESPEED.Utils.tryToCloseStream(input);
        PAGESPEED.Utils.tryToCloseStream(bufferedOut);
      }
    }

    return numBytesWritten;
  },

  /**
   * @param {nsIInputStream} input The stream to copy from.
   * @param {nsIOutputStream} output The stream to copy to.
   * @return {number} The number of bytes written.
   */
  copyCompleteInputToOutput: function(input, output) {
    var totalBytes = 0;
    while (input.available() > 0) {
      var numWritten = PAGESPEED.Utils.copyInputToOutput(
          input, output, input.available(), true);
      if (numWritten <= 0) {
        break;
      }
      totalBytes += numWritten;
    }
    PAGESPEED.Utils.tryToCloseStream(input);
    PAGESPEED.Utils.tryToCloseStream(output);
    return totalBytes;
  },

  /**
   * Counts the number of bytes in a stream by consuming all the bytes
   * in the stream. This method will drain the stream and close it.
   * @param {nsIInputStream} input The stream to copy from.
   * @return {number} The number of bytes in the stream.
   */
  countBytesInStream: function(input) {
    if (!input) return 0;
    var ss = PAGESPEED.Utils.CCIN(
        '@mozilla.org/storagestream;1', 'nsIStorageStream');
    ss.init(4096, 4096, null);
    var totalBytes = 0;
    while (input.available() > 0) {
      var numToWrite = Math.min(4096, input.available());
      var outputStream = ss.getOutputStream(0);
      var actualNumWritten = PAGESPEED.Utils.copyInputToOutput(
        input, outputStream, numToWrite, true);
      PAGESPEED.Utils.tryToCloseStream(outputStream);
      if (actualNumWritten <= 0) {
        break;
      }
      totalBytes += actualNumWritten;
    }
    // Wrap close() in try/catch. It is non-fatal if close()
    // fails, so we just silently discard the exception.
    PAGESPEED.Utils.tryToCloseStream(input);
    return totalBytes;
  },

  /**
   * @param {nsIInputStream} inputStream the stream to generate a hash
   *     for.
   * @return {string} the md5 checksum.
   */
  getMd5HashForInputStream: function(inputStream) {
    var ch = PAGESPEED.Utils.CCIN(
        '@mozilla.org/security/hash;1', 'nsICryptoHash');
    // Use the MD5 algorithm.
    ch.init(ch.MD5);
    // Read the entire file (0xffffffff tells the nsICryptoHash to
    // read until EOF).
    ch.updateFromStream(inputStream, 0xffffffff);
    var hash = ch.finish(false);
    var hexChars = [];
    for (var i = 0, len = hash.length; i < len; i++) {
      var hexChar = hash.charCodeAt(i).toString(16);
      if (hexChar.length == 1) {
        hexChar = '0' + hexChar;
      }
      hexChars.push(hexChar);
    }
    return hexChars.join('');
  },

  /**
   * @param {nsIFile} file The temp file to delete when Firefox exits.
   */
  deleteTemporaryFileOnExit: function(file) {
    var helperAppService = PAGESPEED.Utils.CCSV(
        '@mozilla.org/uriloader/external-helper-app-service;1',
        'nsPIExternalAppLauncher');
    if (!helperAppService) {
      return false;
    }

    helperAppService.deleteTemporaryFileOnExit(file);
    return true;
  },

  /**
   * Open the given link, respecting the user's pref to open in a tab
   * vs a new window.
   * @param {string} url The url to open.
   */
  openLink: function(url) {
    if (PAGESPEED.Utils.getIntPref('browser.link.open_newwindow', 3) == 3) {
      gBrowser.selectedTab = gBrowser.addTab(url);
    } else {
      FirebugChrome.window.open(url, '_blank');
    }
  },

  /**
   * Save a given image
   * @param {string} url The url of image to save.
   */
  saveLink: function(url) {
  	var fileToSave = PAGESPEED.Utils.openSaveAsDialogBox('Save as:', url);

    if (!fileToSave) {
      //User canceled.  Nothing to do.
      return;
    }

    inputStream = PAGESPEED.Utils.getResourceInputStream(url);
    PAGESPEED.Utils.copyCompleteInputToOutput(
      inputStream,
      PAGESPEED.Utils.openFileForWriting(fileToSave.path));
  },

  /**
   * Open 'Save as' dialog to save any sort of files
   * @param {string} message The message is the text to show the user.
   */
  openSaveAsDialogBox: function(message, url) {
    // Open a save as dialog box:
    var fp = PAGESPEED.Utils.CCIN(
      '@mozilla.org/filepicker;1', 'nsIFilePicker');
    const nsIFilePicker = Components.interfaces.nsIFilePicker;

    fp.init(window, message,
            Components.interfaces.nsIFilePicker.modeSave);
    // Set default values
    var fileName = PAGESPEED.Utils.getHumanFileName(url);
    var fileExtension = PAGESPEED.Utils.getFileExtension(url);
    fp.defaultExtension = fileExtension;
    if (fileExtension) {
      fp.defaultString = [fileName, fileExtension].join('.');
    } else {
      fp.defaultString = fileName;
    }
    fp.appendFilters(nsIFilePicker.filterAll|nsIFilePicker.filterImages);

    var result = fp.show();

    if (result != Components.interfaces.nsIFilePicker.returnOK &&
        result != Components.interfaces.nsIFilePicker.returnReplace) {
      // User canceled.  Nothing to do.
      return;
    }
    return fp.file;
  },

  /**
   * Return false if we are to analyze the whole page, or true if we're
   * filtering.
   * @return {boolean} Are we using a filter?
   */
  isUsingFilter: function () {
    return !document.getElementById('psAnalyzeAll').getAttribute('selected');
  },

  /**
   * Format an exception into a string.  Display just enough information
   * to debug the problem without making the error too verbose.
   * @param {Error} ex The exception object.
   * @return {string} String representation of the error.
   */
  formatException: function(ex) {
    // If there is no additional info to add, return ex.toString().
    if (!ex.fileName && !ex.lineNum)
      return ex.toString();

    var resultArr = [ex.toString(),
                     ' (', ex.fileName || '<file unknown>',
                     ':', ex.lineNumber || '<line unknown>',
                     ')'];
    return resultArr.join('');
  },

  quitFirefox: function() {
    var service = PAGESPEED.Utils.CCSV(
        '@mozilla.org/toolkit/app-startup;1',
        'nsIAppStartup');
    service.quit(Components.interfaces.nsIAppStartup.eForceQuit);
  },

  /**
   * Get an nsIURL from a url string.
   * @param {string} urlStr A url.
   * @return {?nsIURL} An nsIURL if the input is valid, null if not.
   */
  urlFromString: function(urlStr) {
    try {
      var uri = PAGESPEED.Utils.CCSV(
          '@mozilla.org/network/io-service;1',
          'nsIIOService').newURI(urlStr, null, null);

      var url = uri.QueryInterface(Components.interfaces.nsIURL);
    } catch (e) {
      // the URI is not an URL
      return null;
    }

    return url;
  },

  /**
   * Some menus and menu items can be enabled or disabled by a preference.
   * For a given XUL element, this function reads a preference
   * and sets the visibility of the XUL element appropriately.
   * @param {string} prefName The boolean preference that enables
   *     this element.
   * @param {string} menuItemId An id for the XUL element, used to
   *     locate it with document.getElementById().
   */
  setItemVisibilityByPref: function(prefName, menuItemId) {
    // Should the menu item be in the menu?  An unset pref means the
    // menu item is disabled.
    var menuItemEnabled = PAGESPEED.Utils.getBoolPref(prefName, false);

    // Collapsed menu items are not shown.
    var menuItem = document.getElementById(menuItemId);
    menuItem.setAttribute('collapsed', !menuItemEnabled);
  },

  /**
   * Display a confirmation dialog (OK/Cancel) to the user.
   * @param {string} title The title of the dialog.
   * @param {string} message The message body of the dialog.
   * @return {boolean} True if the user clicked OK, false otherwise.
   */
  promptConfirm: function(title, message) {
    var promptService = PAGESPEED.Utils.CCSV(
        '@mozilla.org/embedcomp/prompt-service;1',
        'nsIPromptService');
    return promptService.confirm(null, title, message);
  },

  /**
   * @return {string} Name of the operating system (e.g. 'WINNT').
   */
  getOSName: function() {
    var xulRuntime = PAGESPEED.Utils.CCSV(
        '@mozilla.org/xre/app-info;1',
        'nsIXULRuntime');
    return xulRuntime.OS;
  },

  /**
   * @return {string} Name of the platform (e.g. 'WINNT_x86-msvc').
   */
  getPlatformName: function() {
    var xulRuntime = PAGESPEED.Utils.CCSV(
        '@mozilla.org/xre/app-info;1',
        'nsIXULRuntime');
    return xulRuntime.OS + '_' + xulRuntime.XPCOMABI;
  },

  /**
   * @return {nsIFile} Path to the root of the Page Speed add-on.
   */
  getExtensionRoot: function() {
    var fph = PAGESPEED.Utils.getIOService().getProtocolHandler('file')
        .QueryInterface(Components.interfaces.nsIFileProtocolHandler);
    try {
      return fph.getFileFromURLSpec(ADDON_ROOT);
    } catch(e) {
      return null;
    }
  },

  /**
   * @return {string} Name of the native library file for the current
   * platform (e.g. libpagespeed.so).
   */
  getNativeLibraryName: function() {
    var os = PAGESPEED.Utils.getOSName().toLowerCase();
    if (os == 'linux') return 'libpagespeed.so';
    if (os == 'darwin') return 'pagespeed.so';
    if (os == 'winnt') return 'pagespeed.dll';
    return null;
  },

  /**
   * @return {string} Path to the native library file for the current
   * platform.
   */
  getNativeLibraryPath: function() {
    var platformModulePath = PAGESPEED.Utils.getExtensionRoot();
    var libraryName = PAGESPEED.Utils.getNativeLibraryName();
    if (!libraryName || !platformModulePath) return null;
    platformModulePath.append('platform');
    platformModulePath.append(PAGESPEED.Utils.getPlatformName());
    platformModulePath.append('components');
    platformModulePath.append(libraryName);
    return platformModulePath.path;
  },

  /**
   * Get Dom document in JSON format.
   * @return {object} JSON format of DOM document.
   */
  getDocumentDomJson: function(doc) {
    return collectDocument(doc);
  }
};  // End namespace

// Check whether logging is enabled.
try {
  PS_LOG.enabled = PAGESPEED.Utils.getBoolPref(
      'extensions.PageSpeed.enable_console_logging', false);

  PS_LOG.dump = PAGESPEED.Utils.getBoolPref(
      'extensions.PageSpeed.enable_dump_logging', false);

} catch (e) {
  // Code above will fail in unit tests because the prefs service
  // does not yet exist.  Even if the unit test mocks the prefs
  // service, this code runs before that mock is created.  Disable
  // logging for unit tests by default.  Tests that care about
  // logging will enable it on a case-by-case basis.
  PS_LOG.enabled = false;
}

})();  // End closure
