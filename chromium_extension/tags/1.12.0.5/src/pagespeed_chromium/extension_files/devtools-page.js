"use strict";
// Create the visible DevTools panel/tab.
fetchDevToolsAPI(function () {
  if (chromeDevTools && chromeDevTools.panels) {
    chromeDevTools.panels.create("Page Speed",
        "pagespeed-32.png", "pagespeed-panel.html");
  } else {
    alert("Chrome DevTools extension API is not available.");
  }
});
