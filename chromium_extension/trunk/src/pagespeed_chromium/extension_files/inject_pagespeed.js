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

// This script is a content script that should be run within developer
// tools pages that are performing remote debugging (those pages with
// locations that match "http://*/*/devtools.html?*").
//
// The following hackery is required because content scripts only have
// access to a shared DOM, but do not the JavaScript state of the
// page.  We need access to the WebInspector object, so we inject our
// code into the DOM with a script tag.

(function () {

  // addExtensionWrapper will be toStringed and injected into and run directly
  // in the developer tools page. There, for Chrome 16 or newer, use
  // addExtension to insert our extension into the remote devtools page, or for
  // compatibility, access to the WebInspector object directly.

  // TODO (lsong): remove the code using WebInspector directly for compatibility
  // of Chrome 15 or earlier (comment cannot put inside the function).
  function addExtensionWrapper(url)
  {
    if (typeof addExtension === "function") {
      addExtension(url);
      return;
    }

    if (typeof WebInspector === "object" &&
        typeof WebInspector.ElementsPanel === "function" &&
        typeof WebInspector.addExtensions === "function" &&
        typeof WebInspector.addPanel === "function" &&
        WebInspector.socket instanceof WebSocket) {
      if (WebInspector.socket.readyState === WebSocket.OPEN) {
        WebInspector.addExtensions([{ startPage: url }]);
        return;
      }
      function onSocketOpen()
      {
        WebInspector.socket.removeEventListener(onSocketOpen, false);
        WebInspector.addExtensions([{ startPage: url }]);
      }
      WebInspector.socket.addEventListener("open", onSocketOpen, false);
    } else {
      console.error("Page Speed injected into a non-devtools page!");
    }
  }

  // Create a link to the Page Speed landing page.
  var pagespeedURL = escape(chrome.extension.getURL("devtools-page.html"));

  var script = document.createElement("script");
  script.innerText = ("(" + addExtensionWrapper.toString() +
                      ")(unescape('" + pagespeedURL + "'));");
  document.body.appendChild(script);

})();
