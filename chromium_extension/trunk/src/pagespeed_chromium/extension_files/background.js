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
    return function (/*arguments*/) {
      try {
        return func.apply(this, arguments);
      } catch (e) {
        var message = 'Error in Page Speed background page\n: ' + e.stack;
        alert(message + '\n\nPlease file a bug at\n' +
              'http://code.google.com/p/page-speed/issues/');
        console.log(message);
      }
    };
  },

  requestHandler: function (request, sender, sendResponse) {
    if (request.kind === 'runPageSpeed') {
      pagespeed_bg.currentInput = request.input;
      pagespeed_bg.responseChannel = sendResponse;
      chrome.tabs.executeScript(request.tab_id,
                                {file: "content-script.js"});
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
  }

};

chrome.extension.onRequest.addListener(
  pagespeed_bg.withErrorHandler(pagespeed_bg.requestHandler));
