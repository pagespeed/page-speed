/**
 * Copyright 2009 Google Inc.
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
 * @fileoverview Write results to a file.
 *
 * @author Sam Kerner
 */

(function() {  // Begin closure

PAGESPEED.ResultsWriter = {};

/**
 * Write the results of the current page to a file.
 * @param {sring} filePath File path to wrirte to.
 * @param {Object} resultsContainer Results to write.
 */
PAGESPEED.ResultsWriter.writeToFile = function(filePath, resultsContainer) {
  var outStream = null;
  try {
    outStream = PAGESPEED.Utils.openFileForWriting(filePath);
    if (!outStream) {
      PS_LOG('Failed to open file for writing.');
      return false;
    }

    var resultsStream = PAGESPEED.Utils.wrapWithInputStream(
        resultsContainer.toString(2)+'\n');

    PAGESPEED.Utils.copyCompleteInputToOutput(
      resultsStream,
      outStream);

  } catch (error) {
    PS_LOG('ERROR: Exception caught while writing results file "' +
           error + '\n');
  } finally {
    PAGESPEED.Utils.tryToCloseStream(outStream);
  }
};

/**
 * Open a save dialog box, and save the current results.
 * @param {Object} browserTab The tab whose results will be written.
 */
PAGESPEED.ResultsWriter.openJsonExportDialog = function(browserTab) {
  // Open a save as dialog box:
  var fp = PAGESPEED.Utils.CCIN(
      '@mozilla.org/filepicker;1', 'nsIFilePicker');
  fp.init(window, 'Select location of JSON encoded output:',
          Ci.nsIFilePicker.modeSave);

  var result = fp.show();

  if (result != Ci.nsIFilePicker.returnOK &&
      result != Ci.nsIFilePicker.returnReplace) {
    // User canceled.  Nothing to do.
    return;
  }

  // Get the results object.
  var resultsContainer =
    PAGESPEED.ResultsContainer.getResultsContainerByTab(browserTab);

  // Save the selected file.
  PAGESPEED.ResultsWriter.writeToFile(fp.file.path, resultsContainer);
};

/**
 * This function enables or disables the "Export Results" menu
 * based on the presence of a resuls object on a tab.
 * @param {Object} data The properties of data we look at are:
 *     browserTab: The current tab
 */
PAGESPEED.ResultsWriter.updateExportMenu = function(data) {
  var browserTab = data.browserTab;

  var exportMenu = document.getElementById('psExportMenu');
  if (!exportMenu) return;

  exportMenu.disabled =
       !PAGESPEED.ResultsContainer.getResultsContainerByTab(browserTab);
};

// Do not install callbacks in unit tests because the callback holder will
// not exist.
if (PAGESPEED.PageSpeedContext) {

  // When the page speed panel is shown, update the export menu's status.
  PAGESPEED.PageSpeedContext.callbacks.showPageSpeed.addCallback(
      PAGESPEED.ResultsWriter.updateExportMenu);
}

})();  // End closure
