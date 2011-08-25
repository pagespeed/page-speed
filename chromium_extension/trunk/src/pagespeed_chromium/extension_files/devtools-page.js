"use strict";
// Create the visible DevTools panel/tab.
fetchDevToolsAPI(function () {
  webInspector.panels.create("Page Speed", "pagespeed-32.png",
                             "pagespeed-panel.html");
});
