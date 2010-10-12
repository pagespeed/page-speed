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

  // Wrap a function with an error handler.  Given a function, return a new
  // function that behaves the same but catches and logs errors thrown by the
  // wrapped function.
  withErrorHandler: function (func) {
    return function (/*arguments*/) {
      try {
        return func.apply(this, arguments);
      } catch (e) {
        var message = e.toString();
        alert("Error in Page Speed background page: " + message +
              '\n\nPlease file a bug at\n' +
              'http://code.google.com/p/page-speed/issues/');
      }
    };
  },

  requestHandler: function (request, sender, sendResponse) {
    var response = null;
    try {
      if (request.kind === 'openUrl') {
        chrome.tabs.create({url: request.url});
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

};

chrome.extension.onRequest.addListener(
  pagespeed_bg.withErrorHandler(pagespeed_bg.requestHandler));
