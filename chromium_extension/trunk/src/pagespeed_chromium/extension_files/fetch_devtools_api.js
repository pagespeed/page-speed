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

function fetchDevToolsAPI (callback) {
  // Attempt to load the extension API from each source file in order,
  // until one succeeds.  On success, invoke the callback.
  var loadExtensionAPI = function (srcArray) {
    // ExtensionAPI.js expects WebInspector to be an object.
    window.WebInspector = { };

    if (srcArray && srcArray.length > 0) {
      var script = document.createElement("script");
      script.async = false;

      // Once ExtensionAPI.js has been injected into the page,
      // WebInspector.injectedExtensionAPI() will be available.
      // Inject init_devtools_api to call the API initialization
      // function.
      script.onload = script.onerror = function () {
        if (WebInspector.injectedExtensionAPI) {
          // Fetch successfull... call init function and callback.
          WebInspector.injectedExtensionAPI(null, null, "pagespeed");
          delete window.WebInspector;
          callback();
        } else {
          // Fetch failed... try next URL.
          loadExtensionAPI(srcArray.slice(1));
        }
      };
      script.src = srcArray[0];
      document.head.appendChild(script);
    } else {
      alert("Whoops, it looks like the DevTools Extensions API is " +
            "unavailable from the remote browser.\n\n" +
            "ExtensionsAPI.js must be be available under Chrome/" +
            "Application/<version>/Resources/Inspector/devTools.css, " +
            "which is currently only the case for local builds that " +
            "have 'debug_devtools': 1 set in GYP config.  If you " +
            "want to play with it, copy the file from " +
            "http://trac.webkit.org/browser/trunk/Source/WebCore/" +
            "inspector/front-end/ExtensionAPI.js");
      delete window.WebInspector;
    }
  }

  // If the webInspector API is unavailable, then we need to fetch it.
  if (!window.webInspector) {

    // Try fetching from several different URLs:
    loadExtensionAPI([
      // A. document.referrer should be the URL of the remote
      //    devtools.  ExtensionAPI.js should reside at the same
      //    location.
      document.referrer.replace(/devtools.html?.*$/, "ExtensionAPI.js"),

      // B. DevTools.js includes ExtensionAPI plus a bunch of stuff we
      //    don't need.
      document.referrer.replace(/devtools.html?.*$/, "DevTools.js"),

      // C. Fall back on fetching the most recent version from trac.
      "http://trac.webkit.org/browser/trunk/Source/WebCore/inspector/" +
      "front-end/ExtensionAPI.js?format=txt",
    ]);
  } else {
    callback();
  }
}
