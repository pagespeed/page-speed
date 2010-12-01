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

function withErrorHandler(func) {
  return function (/*arguments*/) {
    try {
      return func.apply(this, arguments);
    } catch (e) {
      var message = 'Error in Page Speed content script: ' + e;
      alert(message + '\n\nPlease file a bug at\n' +
            'http://code.google.com/p/page-speed/issues/');
      chrome.extension.sendRequest({kind: 'error', message: message});
    }
  };
}

function receiveInput(response) {
  if (!response) {
    throw new Error('No response to getInput request.');
  }

  // Load the Page Speed NaCl module.
  var pagespeed_module = document.createElement('embed');
  pagespeed_module.setAttribute('name', 'nacl_module');
  pagespeed_module.setAttribute('width', 0);
  pagespeed_module.setAttribute('height', 0);
  var nexes = ('x86-32: ' + chrome.extension.getURL('pagespeed_ia32.nexe') +
               '\nx86-64: ' + chrome.extension.getURL('pagespeed_x64.nexe') +
               '\nARM: ' + chrome.extension.getURL('pagespeed_arm.nexe'));
  pagespeed_module.setAttribute('nexes', nexes);
  pagespeed_module.setAttribute('type', 'application/x-nacl-srpc');

  // Add the module to the body so that the NaCl module will load.
  // TODO(mdsteele): Find a way to load the module without modifying the body.
  var body = document.getElementsByTagName('body')[0];
  body.appendChild(pagespeed_module);

  var passInputToPageSpeedModule = function () {
    // Feed the HAR data into the NaCl module.  We have to do this a piece at a
    // time, because SRPC currently can't handle strings larger than one or two
    // dozen kilobytes.
    var har_string = JSON.stringify(response.har);
    var har_length = har_string.length;
    var kChunkSize = 8192;
    for (var start = 0; start < har_length; start += kChunkSize) {
      pagespeed_module.appendInput(har_string.substr(start, kChunkSize));
    }

    // Run the rules.
    pagespeed_module.runPageSpeed(document, response.analyze);
  
    // Get the result data back from the NaCl module.  Again, this must be done
    // a piece at a time.
    var output_chunks = [];
    while (true) {
      var piece = pagespeed_module.readMoreOutput();
      if (typeof(piece) !== 'string') {
        break;
      }
      output_chunks.push(piece);
    }
    var output_string = output_chunks.join('');

    // Take the module back out of the body.
    body.removeChild(pagespeed_module);
  
    // Send the results back to the extension.
    chrome.extension.sendRequest({
      kind: 'putResults',
      results: {
        analyze: response.analyze,
        results: JSON.parse(output_string)
      }
    });
  };

  var timesTried = 0;
  var tryPassingInput = function () {
    try {
      // If the module is ready, this will have no effect; if not, it will
      // throw an error.
      pagespeed_module.appendInput('');
    } catch (e) {
      // If we've been doing this for a few seconds with no success, give up.
      ++timesTried;
      if (timesTried >= 20) {
        throw e;
      }
      // The module isn't ready yet, so wait for a short time and try again.
      // TODO(mdsteele): Is there a way to know exactly when the module has
      //   loaded?  I haven't been able to get onLoad handlers to work.
      setTimeout(withErrorHandler(tryPassingInput), 100);
      return;
    }
    // The module seems to be ready now.
    passInputToPageSpeedModule();
  };
  tryPassingInput();
}

chrome.extension.sendRequest({kind: 'getInput'},
                             withErrorHandler(receiveInput));
