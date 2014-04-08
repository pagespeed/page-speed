/**
 * Copyright 2010 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * @fileoverview Installs a pagespeed object on the page's window,
 * which allows the web page to invoke Page Speed programatically.
 *
 * @author Bryan McQuade
 */

(function() {  // Begin closure

function onNewWindow(win) {
  var pagespeed = {};
  pagespeed.computeFormattedResults = PAGESPEED.NativeLibrary.invokeNativeLibraryAndFormatResults;
  pagespeed.computeResults = PAGESPEED.NativeLibrary.invokeNativeLibraryAndComputeResults;
  win.wrappedJSObject.pagespeed = pagespeed;
}

try {
  if (PAGESPEED.Utils.getBoolPref(
          'extensions.PageSpeed.enable_client_api', false)) {
    var collector = PAGESPEED.Utils.getComponentCollector();
    collector.addNewWindowListener(onNewWindow);
  }
} catch (e) {
  PS_LOG('Failed to install new window listener.');
}

})();  // End closure
