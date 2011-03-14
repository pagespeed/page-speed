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

  // TODO(mdsteele): What if there were multiple pending runPageSpeed requests
  //     at once?  Maybe we should have a dict here instead of one variable.
  // The input that was given to runPageSpeed; we have to store it so that the
  // content script can request it asynchronously.
  currentInput: null,

  // The channel on which to respond to requests to runPageSpeed; we have to
  // store it because we have to respond to those requests asynchronously.
  responseChannel: null,

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
        var message = 'Error in Page Speed background page\n: ' + e.stack;
        alert(message + '\n\nPlease file a bug at\n' +
              'http://code.google.com/p/page-speed/issues/');
        console.log(message);
      }
    };
  },

  requestHandler: function (request, sender, sendResponse) {
    // TODO: how do we want to handle re-entrancy here? Since the
    // background page runs in a different thread/process we can't
    // count on single-threaded execution relative to the JS executing
    // in the content page. We also need to be careful if we use
    // global state as an indicator that a request is in process,
    // since a request that accidentally fails part way through
    // processing will lock up processing for all subsequent requests.
    if (request.kind === 'runPageSpeed') {
      // First we need to fetch any resources for which Chrome
      // Developer Tools Extension APIs only gave us partial data.
      pagespeed_bg.currentInput = request.input;
      pagespeed_bg.responseChannel = sendResponse;
      pagespeed_bg.fetchPartialResources_(request);
    } else {
      var response = null;
      try {
        if (request.kind === 'openUrl') {
          chrome.tabs.create({url: request.url});
        } else if (request.kind === 'getInput') {
          response = pagespeed_bg.currentInput;
        } else if (request.kind === 'putResults') {
          pagespeed_bg.currentInput = null;
          pagespeed_bg.responseChannel(request.results);
          pagespeed_bg.responseChannel = null;
        } else if (request.kind === 'error') {
          if (pagespeed_bg.responseChannel) {
            pagespeed_bg.currentInput = null;
            pagespeed_bg.responseChannel({error_message: request.message});
            pagespeed_bg.responseChannel = null;
          }
        } else {
          throw new Error("Unknown request kind: kind=" + request.kind +
                          " sender=" + JSON.stringify(sender));
        }
      } finally {
        // We should always send a response, even if it's empty.  See:
        //   http://code.google.com/chrome/extensions/messaging.html#simple
        sendResponse(response);
      }
    }
  },

  fetchPartialResources_: function (request) {
    var fetchContext = {}

    // We track in-progress requests in a map, so we know when there
    // are no resources left being fetched. This is a map from URL to
    // the XHR instance for that URL.
    fetchContext.xhrs = {};

    // Add a key to count the number of outstanding resources.
    fetchContext.numOutstandingResources = 0;

    // Hold on to the current request instance.
    fetchContext.request = request;

    var har = request.input.har.log;
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

      // We only re-issue requests for GETs. If not a GET, skip it.
      if (entry.request.method !== 'GET') {
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
          (!entry.response.text || entry.response.text.length == 0) &&
          entry.response.bodySize !== 0;

      if (!hasNoResponseHeaders && !is304Response && !isMissingResponseBody) {
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

      xhr.onreadystatechange = pagespeed_bg.withErrorHandler(
          pagespeed_bg.onReadyStateChange,
          xhr, entry, fetchContext);
      try {
        xhr.open("GET", url, true);
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
      delete fetchContext.request;
      pagespeed_bg.executeContentScript(request);
    }
  },

  executeContentScript: function(request) {
    if (request.input === pagespeed_bg.currentInput) {
      // Only run the content script if this is part of processing for
      // a request that hasn't been cancelled.
      chrome.tabs.executeScript(request.tab_id,
                                {file: "content-script.js"});
    }
  },

  onReadyStateChange: function (xhr, entry, fetchContext) {
    if (pagespeed_bg.currentInput !== fetchContext.request.input) {
      // We're processing a callback for an old request. Ignore it.
      return;
    }

    if (xhr.readyState !== 4) {
      // Non-final state, so return.
      return;
    }

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
      var request = fetchContext.request;
      delete fetchContext.request;
      pagespeed_bg.executeContentScript(request);
    }
  },

  onXhrResponse: function(xhr, entry) {
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

  updateResponseHeaders: function(xhr, entry) {
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

  updateResponseBody: function(xhr, entry) {
    var content = entry.response.content;
    if (!content || (content.text && content.text.length > 0)) {
      // Either there's no content entry, or we already have a
      // response body, so there's nothing more to do.
      return;
    }

    var body = xhr.responseText;
    var contentType = xhr.getResponseHeader('Content-Type');
    var encoding;
    if (content && content.encoding) {
      encoding = content.encoding;
    }
    if (encoding === 'base64' || contentType.substr(0, 6) === 'image/') {
      encoding = 'base64';

      // TODO: base64-encode. We can't use btoa() since it does not
      // work on binary data. We likely need to call a native method
      // on our NPAPI plugin for this.

      // For now, abort since we can't properly encode this response.
      return;
    }

    // Update the content fields with the new data.
    content.text = body;
    content.size = body.length;
    if (encoding) {
      content.encoding = encoding;
    }
  }

};

chrome.extension.onRequest.addListener(
  pagespeed_bg.withErrorHandler(pagespeed_bg.requestHandler));
