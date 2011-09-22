"use strict";
// Create the visible DevTools panel/tab.
fetchDevToolsAPI(function () {
  if ("devtools" in chrome.experimental &&
      chrome.experimental.devtools.panel) {
    chrome.experimental.devtools.panels.create("Page Speed",
        "pagespeed-32.png", "pagespeed-panel.html");

  } else {
    webInspector.panels.create("Page Speed", "pagespeed-32.png",
                               "pagespeed-panel.html");
  }
});
