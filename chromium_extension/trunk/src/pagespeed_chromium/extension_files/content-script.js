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

// Wrap a function with an error handler.  Given a function, return a new
// function that behaves the same but catches and logs errors thrown by the
// wrapped function.
function withErrorHandler(func) {
  return function (/*arguments*/) {
    try {
      return func.apply(this, arguments);
    } catch (e) {
      var message = 'Error in Page Speed content script:\n ' + e.stack;
      alert(message + '\n\nPlease file a bug at\n' +
            'http://code.google.com/p/page-speed/issues/');
      chrome.extension.sendRequest({kind: 'error', message: message});
    }
  };
}

// Set the status text for the DevTools panel inspecting this page.
function setStatusText(message) {
  chrome.extension.sendRequest({kind: 'setStatusText', message: message});
}

function receiveInput(response) {
  if (!response) {
    throw new Error('No response to getInput request.');
  }

  setStatusText('Loading Page Speed module...');
  // Load the Page Speed NaCl module.
  var pagespeed_module = document.createElement('embed');
  pagespeed_module.setAttribute('width', 0);
  pagespeed_module.setAttribute('height', 0);
  pagespeed_module.setAttribute('type', 'application/x-page-speed');

  // Add the module to the body so that the NaCl module will load.
  // TODO(mdsteele): Find a way to load the module without modifying the body.
  var body = document.getElementsByTagName('body')[0];
  // If there's no body tag, maybe this is a frameset page.  If so, we're going
  // to have to give up, because adding an embed tag isn't going to load the
  // plugin.
  if (!body && document.getElementsByTagName('frameset')[0]) {
    chrome.extension.sendRequest({kind: 'error', reason: 'frameset'});
    return;
  }
  body.appendChild(pagespeed_module);

  var passInputToPageSpeedModule = function () {
    var har_string = JSON.stringify(response.har);

    // Determine the locale of the browser; for details, see
    // http://code.google.com/chrome/extensions/i18n.html#overview-predefined
    var locale = chrome.i18n.getMessage('@@ui_locale');

    // Run the rules.
    setStatusText(chrome.i18n.getMessage('running_rules'));
    var output_string = pagespeed_module.runPageSpeed(
      har_string, document, response.analyze, locale);

    // Take the module back out of the body.
    body.removeChild(pagespeed_module);
  
    // Send the results back to the extension.
    setStatusText('Sending results to DevTools panel...');
    chrome.extension.sendRequest({
      kind: 'putResults',
      results: {
        analyze: response.analyze,
        results: JSON.parse(output_string)
      }
    });
  };

  var timesTried = 0;
  var tryPinging = function () {
    try {
      // If the module is ready, this will have no effect; if not, it will
      // throw an error.
      pagespeed_module.ping();
    } catch (e) {
      // If we've been doing this for a few seconds with no success, give up.
      ++timesTried;
      if (timesTried >= 20) {
        chrome.extension.sendRequest({
          kind: 'error',
          message: 'Could not load Page Speed plugin:\n' + e.stack,
          reason: 'moduleDidNotLoad'
        });
        // Take the module back out of the body.
        body.removeChild(pagespeed_module);
      } else {
        // The module isn't ready yet, so wait for a short time and try again.
        // TODO(mdsteele): Is there a way to know exactly when the module has
        //   loaded?  I haven't been able to get onLoad handlers to work.
        setTimeout(withErrorHandler(tryPinging), 100);
      }
      return;
    }
    // The module seems to be ready now.
    passInputToPageSpeedModule();
  };
  tryPinging();
}

chrome.extension.sendRequest({kind: 'getInput'},
                             withErrorHandler(receiveInput));
