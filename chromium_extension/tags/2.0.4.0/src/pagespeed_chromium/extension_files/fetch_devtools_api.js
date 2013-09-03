// Copyright 2011 Google Inc. All Rights Reserved.
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

function fetchDevToolsAPI (real_callback) {
  // Install chrome devtools alias. If the devtools is not
  // immediately available, we will return false.
  var installDevToolsAlias = function () {
    if (!window.chrome) {
      return false;
    }

    // Prefer webInspector for now, since old version of chrome does not like to
    // use chrome.devtools.
    if (window.webInspector) {
      console.log("Using webInspector instead of chrome.devtools.");
      window.chromeDevTools = window.webInspector;
      return true;
    }

    var has_experimental_devtools = "devtools" in chrome.experimental &&
          "panels" in chrome.experimental.devtools;
    if (has_experimental_devtools) {
      console.log("Using chrome.experimental.devtools.");
      window.chromeDevTools = chrome.experimental.devtools;
      return true;
    }

    var has_devtools = false;
    try {
      has_devtools = "devtools" in chrome && "panels" in chrome.devtools;
    } catch (e) {
      // Chrome 15 or earlier will throw exception for accessing
      // chrome.devtools. chrome.devtools can only be used in extension
      // processes.
      has_devtools = false;
    }

    if (has_devtools) {
      console.log("Using chrome.devtools.");
      window.chromeDevTools = chrome.devtools;
      return true;
    }

    return false;
  }

  // Attempt to load the extension API from src url. On success, invoke the
  // callback.
  var loadExtensionAPI = function (src) {
    var script = document.createElement("script");
    script.async = false;
    script.onerror = function () {
      console.log("ERROR: failed to fetch devtools_extension_api from " + src);
      alert("ERROR, failed to fetch devtools_extension_api.js.");
    };

    script.onload = function () {
      if (installDevToolsAlias() ) {
        // Invoke the  callback after a timeout. Otherwise, we may be trying to
        // create our Page Speed panel before the DevTools panels are
        // initialized. Note, we hard coded 100 milliseconds. If we are
        // experiencing problems, we need to increase the timeout value.
        window.setTimeout(real_callback, 100);
      } else {
        console.log("ERROR: devtools_extension_api not available from " + src);
        alert("ERROR: devtools extension API is not available.");
      }
    };
    script.src = src;
    document.head.appendChild(script);
  }


  // If we do not need to load the extension APIs, use the real callback
  // function without delay.
  if (installDevToolsAlias()) {
    real_callback();
  } else {
    loadExtensionAPI(
      //  document.referrer should be the URL of the remote devtools.
      document.referrer.replace(/devtools.html?.*$/,
                                "devtools_extension_api.js")
    );
  }
}
