// Copyright 2010 Google Inc. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

"use strict";

var pagespeed_bg = {

  // Map from DevTools tab IDs to client objects.  Each client object has the
  // following fields:
  //     analyze: a string indicating whether to analyze ads/content/etc.
  //     har: the page HAR, in JSON form
  //     port: a port object connected to this client's DevTools page
  activeClients: {},

  // Wrap a function with an error handler.  Given a function, return a new
  // function that behaves the same but catches and logs errors thrown by the
  // wrapped function.
  withErrorHandler: function (func) {
    // Remove the first arg.
    var boundArgs = Array.prototype.slice.call(arguments, 1);
    return function (/*arguments*/) {
      try {
        // Prepend boundArgs to the new args.
        var newArgs = Array.prototype.slice.call(arguments);
        Array.prototype.unshift.apply(newArgs, boundArgs);
        return func.apply(this, newArgs);
      } catch (e) {
        var message = 'Error in Page Speed background page:\n ' + e.stack;
        alert(message + '\n\nPlease file a bug at\n' +
              'http://code.google.com/p/page-speed/issues/');
        console.log(message);
      }
    };
  },

  // Given a client object, return true if it is still active, or false if it
  // has been cancelled.
  isClientStillActive: function (client) {
    return (client.port &&
            client.port.sender.tab.id in pagespeed_bg.activeClients);
  },

  // Handle connections from DevTools panels.
  connectHandler: function (port) {
    port.onMessage.addListener(pagespeed_bg.withErrorHandler(
      pagespeed_bg.messageHandler, port));
  },

  // Handle messages from DevTools panels.
  messageHandler: function (port, request) {
    var tab_id = port.sender.tab.id;
    if (request.kind === 'openUrl') {
      chrome.tabs.create({url: request.url});
    } else if (request.kind === 'fetchPartialResources') {
      var client = {
        analyze: request.analyze,
        har: request.har,
        port: port,
      };
      pagespeed_bg.activeClients[tab_id] = client;
      port.onDisconnect.addListener(pagespeed_bg.withErrorHandler(function () {
        delete pagespeed_bg.activeClients[tab_id];
      }));
      pagespeed_bg.fetchPartialResources_(client);
    } else if (request.kind === 'cancelRun') {
      delete pagespeed_bg.activeClients[tab_id];
    } else {
      throw new Error('Unknown message kind:' + request.kind);
    }
  },

  fetchPartialResources_: function (client) {
    var fetchContext = {}

    // We track in-progress requests in a map, so we know when there
    // are no resources left being fetched. This is a map from URL to
    // the XHR instance for that URL.
    fetchContext.xhrs = {};

    // Add a key to count the number of outstanding resources.
    fetchContext.numOutstandingResources = 0;

    // Hold on to the current client instance.
    fetchContext.client = client;

    var har = client.har.log;
    for (var i = 0, len = har.entries.length; i < len; ++i) {
      var entry = har.entries[i];

      // First check to see that basic data is available for this
      // entry.
      if (!entry || !entry.request || !entry.response) {
        var url = '<unknown_url>.';
        if (entry && entry.request && entry.request.url) {
          url = entry.request.url;
        }
        console.log('Incomplete resource ' + url);
        continue;
      }

      var url = entry.request.url;

      // The HAR sometimes includes entries for about:blank and other
      // URLs we don't care about. Ignore them.
      if (url.substr(0, 4) !== 'http') {
        continue;
      }

      var content = entry.response.content;
      var isRawBinaryResponse = content &&
          !pagespeed_bg.isTextMimeType(content.mimeType) &&
          content.encoding !== 'base64';
      if (isRawBinaryResponse) {
        // Chrome Developer Tools has a bug where it sometimes
        // attempts to return a non-base64 encoded response for binary
        // data. In this case we must discard the response body, since
        // it's not safe to attempt to pass to the native client
        // module. Note that it would be better to try to convert the
        // binary string to a base64-encoded string directly here, but
        // it's not clear that that is possible, so we instead clear
        // the data and refetch as binary data via an XHR.
        content.text = '';
        content.encoding = '';
        content.size = 0;
      }

      // We only re-issue requests for GETs. If not a GET, skip it.
      if (entry.request.method !== 'GET') {
        continue;
      }

      // We do not request redirect URLs, because XHR will follow redirections.
      if (entry.response.status === 301 || entry.response.status === 302 ||
          entry.response.status === 303 || entry.response.status === 307) {
        continue;
      }

      // There are 3 known cases where Chrome Developer Tools
      // Extension APIs give us partial data: 304 responses (partial
      // HTTP headers), responses from cache (in which case there are
      // no HTTP headers), and responses without bodies that have a
      // content length greater than zero.
      var is304Response = (entry.response.status === 304);
      var hasNoResponseHeaders =
          !entry.response.headers || entry.response.headers.length == 0;
      var isMissingResponseBody =
          (!content || !content.text || content.text.length == 0) &&
          entry.response.bodySize !== 0;

      if (!hasNoResponseHeaders &&
          !is304Response &&
          !isMissingResponseBody &&
          !isRawBinaryResponse) {
        // Looks like we should have all the data for this
        // response. No need to re-fetch.
        continue;
      }

      if (url in fetchContext.xhrs) {
        // Only fetch each resource once. Sometimes the HAR file will
        // contain more than one entry for the same resource. We
        // process the first entry for a given URL, which is the only
        // one that the Page Speed library will analyze.
        continue;
      }

      // Create an XHR to fetch the resource. Add it to the
      // fetchContext before invoking send() in case the response
      // somehow arrives synchronously (IE's message pump has strange
      // behavior so it's best that we program defensively here).
      var xhr = new XMLHttpRequest();
      fetchContext.xhrs[url] = xhr;
      fetchContext.numOutstandingResources++;

      // Abort any requests that take longer than 5 seconds, since
      // some requests are "hanging GETs" that never return.
      var timeoutCallbackId = setTimeout(
          pagespeed_bg.withErrorHandler(pagespeed_bg.abortXmlHttpRequest_,
                                        xhr, url),
          5000);

      xhr.onreadystatechange = pagespeed_bg.withErrorHandler(
          pagespeed_bg.onReadyStateChange,
          xhr, entry, fetchContext, timeoutCallbackId);
      try {
        xhr.open('GET', url, true);
        if (!pagespeed_bg.isTextMimeType(content.mimeType)) {
          // Request to get the response data in the form of an
          // ArrayBuffer.  If we don't do this, the XHR tends to try
          // to interpret binary data as UTF8-encoded text, which
          // doesn't work out very well.
          xhr.responseType = 'arraybuffer';
        }
        xhr.send();
      } catch (e) {
        console.log('Failed to request resource ' + url);
        delete fetchContext.xhrs[url];
        fetchContext.numOutstandingResources--;
      }
    }

    if (fetchContext.numOutstandingResources == 0) {
      // We have no outstanding resources being fetched, so move on to
      // the next stage of processing.
      delete fetchContext.client;
      pagespeed_bg.sendCollectedInputMsg(client);
    }
  },

  abortXmlHttpRequest_: function (xhr, url) {
    console.log('Aborting XHR for ' + url);
    // Calling xhr.abort() will trigger a callback to
    // onReadyStateChange, where the XHR has a status code of
    // zero. According to the XHR spec, this may change at some time
    // in the future, so we need to watch for that and update the code
    // if necessary.
    xhr.abort();
  },

  sendCollectedInputMsg: function (client) {
    if (pagespeed_bg.isClientStillActive(client)) {
      // Only send the fetch complete message if this is part of
      // processing for a client that hasn't been cancelled.
      client.port.postMessage({
        kind: 'partialResourcesFetched',
        analyze: client.analyze,
        har: client.har
      });
    }
  },

  onReadyStateChange: function (xhr, entry, fetchContext, timeoutCallbackId) {
    if (!pagespeed_bg.isClientStillActive(fetchContext.client)) {
      // We're processing a callback for an old client. Ignore it.
      return;
    }

    if (xhr.readyState !== 4) {
      // Non-final state, so return.
      return;
    }

    clearTimeout(timeoutCallbackId);

    var url = entry.request.url;
    if (!url in fetchContext.xhrs) {
      console.log('No such xhr ' + url);
      return;
    }

    delete fetchContext.xhrs[url];
    fetchContext.numOutstandingResources--;

    // Invoke the callback with an error handler so we continue on and
    // finish the next stage of processing, even if one of our XHR
    // responses doesn't process correctly.
    var wrappedResponseHandler = pagespeed_bg.withErrorHandler(
        pagespeed_bg.onXhrResponse);
    wrappedResponseHandler(xhr, entry);

    if (fetchContext.numOutstandingResources == 0) {
      // We're done fetching outstanding resources, so move on to the
      // next phase of processing.
      var client = fetchContext.client;
      delete fetchContext.client;
      pagespeed_bg.sendCollectedInputMsg(client);
    }
  },

  onXhrResponse: function (xhr, entry) {
    var url = entry.request.url;

    // The server may 304 if the browser issues a conditional get,
    // however the lower-level network stack will synthesize a proper
    // 200 response for the XHR.
    if (xhr.status !== 200) {
      console.log('Got non-200 response ' + xhr.status + ' for ' + url);
      return;
    }

    pagespeed_bg.updateResponseHeaders(xhr, entry);
    pagespeed_bg.updateResponseBody(xhr, entry);
    entry.response.status = 200;
  },

  updateResponseHeaders: function (xhr, entry) {
    function getHeaderKeyValue(headerLine) {
      // Find the first colon and split key, value at that point.
      var separatorIdx = headerLine.indexOf(':');
      if (separatorIdx === -1) {
        console.log('Failed to get valid header from ' + headerLine);
        return null;
      }
      var k = headerLine.substr(0, separatorIdx).trim().toLowerCase();
      var v = headerLine.substr(separatorIdx + 1).trim();
      return { name: k, value: v };
    }

    var is304Response = (entry.response.status === 304);
    var hasNoResponseHeaders =
        !entry.response.headers || entry.response.headers.length == 0;
    if (!is304Response && !hasNoResponseHeaders) {
      // The entry isn't one that meets the criteria for needing
      // updated headers, so don't update headers.
      return;
    }

    // We'll store all headers in the allResponseHeaders map, which is
    // a map from header name to an array of values. We store an array
    // of values rather than a single value since HTTP headers with
    // the same name are allowed to appear multiple times in a single
    // response.
    var allResponseHeaders = {};

    // Map the HAR headers, which are stored in an array, into a map,
    // so we can easily look headers up by keyname.
    var harHeadersArray = entry.response.headers;
    if (!harHeadersArray) {
      harHeadersArray = [];
    }
    for (var i = 0, len = harHeadersArray.length; i < len; ++i) {
      var kv = harHeadersArray[i];
      var key = kv.name.toLowerCase();
      if (!allResponseHeaders[key]) {
        allResponseHeaders[key] = [];
      }
      allResponseHeaders[key].push(kv.value);
    }

    // Get the headers from the XHR.
    var xhrResponseHeadersStr = xhr.getAllResponseHeaders().split('\n');
    if (xhrResponseHeadersStr[xhrResponseHeadersStr.length - 1].trim() === '') {
      // Remove the last entry, which is an empty newline.
      xhrResponseHeadersStr.pop();
    }

    // Remove any header entries from the HAR for which there is also
    // an entry in the XHR. The XHR headers are more accurate.
    for (var i = 0, len = xhrResponseHeadersStr.length; i < len; ++i) {
      var kv = getHeaderKeyValue(xhrResponseHeadersStr[i]);
      if (kv) {
        allResponseHeaders[kv.name] = [];
      }
    }

    // Add the XHR headers to the allResponseHeaders map.
    for (var i = 0, len = xhrResponseHeadersStr.length; i < len; ++i) {
      var kv = getHeaderKeyValue(xhrResponseHeadersStr[i]);
      if (kv) {
        allResponseHeaders[kv.name].push(kv.value);
      }
    }

    // Construct a new HAR-style header array that contains all the
    // headers.
    var responseHeadersArray = [];
    for (var key in allResponseHeaders) {
      for (var i = 0, len = allResponseHeaders[key].length; i < len; ++i) {
        responseHeadersArray.push(
            {name: key, value: allResponseHeaders[key][i]});
      }
    }

    entry.response.headers = responseHeadersArray;
  },

  updateResponseBody: function (xhr, entry) {
    var content = entry.response.content;
    if (!content || (content.text && content.text.length > 0)) {
      // Either there's no content entry, or we already have a
      // response body, so there's nothing more to do.
      return;
    }

    var bodyEncoded;
    var encoding;

    if (xhr.responseType == 'arraybuffer') {
      // Get the content as raw binary data.  Since we set
      // xhr.responseType to 'arraybuffer', xhr.response will be an
      // ArrayBuffer object (except that it may be null for empty
      // responses, in which case we create an empty Uint8Array).
      var bodyBytes = (xhr.response === null ? new Uint8Array() :
                       new Uint8Array(xhr.response));
      var bodySize = bodyBytes.length;
      // Encode the binary content into base64.  For now, the
      // following mess is the easiest way I know how to do this.
      // Hopefully someday there will be an easier way. Note that this
      // mechanism does not handle multi-byte characters correctly,
      // however, we attempt to only fetch binary resources as
      // arraybuffers so this should not be a problem for us.
      var bodyCharacters = [];
      for (var index = 0; index < bodySize; ++index) {
        bodyCharacters.push(String.fromCharCode(bodyBytes[index]));
      }
      bodyEncoded = btoa(bodyCharacters.join(''));
      encoding = 'base64';
    } else {
      bodyEncoded = xhr.responseText;
      encoding = '';
      bodySize = bodyEncoded.length;
    }

    // Update the content fields with the new data.
    content.text = bodyEncoded;
    content.size = bodySize;
    content.encoding = encoding;
  },

  isTextMimeType: function(mimeType) {
    if (!mimeType || mimeType.length == 0) return false;
    mimeType = mimeType.toLowerCase();

    if (mimeType.substr(0, 5) == 'text/') return true;

    // See http://www.w3.org/TR/xhtml-media-types/
    if (mimeType.indexOf('/xhtml') > -1) return true;
    if (mimeType.indexOf('/xml') > -1) return true;
    if (mimeType.indexOf('+xml') > -1) return true;

    // See http://www.w3.org/wiki/HTML/Elements/script
    if (mimeType.indexOf('javascript') > -1) return true;
    if (mimeType.indexOf('ecmascript') > -1) return true;
    if (mimeType.indexOf('/jscript') > -1) return true;
    if (mimeType.indexOf('/livescript') > -1) return true;

    // We assume that everything else is binary. This includes
    // application/* not covered by the cases above, as well as
    // image/*.
    return false;
  },

};

pagespeed_bg.withErrorHandler(function () {

  // Listen for connections from DevTools panels:
  chrome.extension.onConnect.addListener(
    pagespeed_bg.withErrorHandler(pagespeed_bg.connectHandler));

})();
