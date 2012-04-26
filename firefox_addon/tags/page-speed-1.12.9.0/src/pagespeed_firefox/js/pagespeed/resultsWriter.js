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
 * @param {string} filePath File path to write to.
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
        resultsContainer.toString(2) + '\n');

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
 * @param {Object} resultsContainer The results to be exported.
 */
PAGESPEED.ResultsWriter.openJsonExportDialog = function(resultsContainer) {
  var fileToSave = PAGESPEED.Utils.openSaveAsDialogBox(
      'Select location of JSON encoded output:', 'pagespeed_results.json');
  if (!fileToSave) {
    // User canceled.  Nothing to do.
    return;
  }

  // Save the selected file.
  PAGESPEED.ResultsWriter.writeToFile(fileToSave.path, resultsContainer);
};

/**
 * Update an export menu item to have the proper text showing where
 * the scores/results will be sent and enable if appropriate.
 *
 * @param {Object} beacon The beacon to send.
 * @param {String} menuItemId Id of the menu item to update.
 * @param {String} message Message to use for the menu item.
 */
var updateExportMenuItem = function(beacon, menuItemId, message) {
  var menuItem = document.getElementById(menuItemId);
  if (menuItem) {
    var domain = beacon.getBeaconDomain();
    menuItem.setAttribute('collapsed', domain == null);
    if (domain != null) {
      menuItem.setAttribute('label', [message, domain].join(''));
    }
  }
};

/**
 * This function enables or disables the "Export Results" menu
 * based on the presence of a resuls object on a tab.
 * @param {Object} data The properties of data we look at are:
 *     browserTab: The current tab.
 */
PAGESPEED.ResultsWriter.updateExportMenu = function(data) {
  var browserTab = data.browserTab;

  var exportMenu = document.getElementById('psExportMenu');
  if (!exportMenu) {
    PS_LOG('Failed to find export menu XUL element.');
    return;
  }

  if (!PAGESPEED.ResultsContainer.getResultsContainerByTab(browserTab)) {
    exportMenu.setAttribute('disabled', 'true');
    return;
  }

  exportMenu.removeAttribute('disabled');

  updateExportMenuItem(PAGESPEED.minimalBeacon,
		       'psMinimalBeacon',
		       'Send Scores to ');

  updateExportMenuItem(PAGESPEED.fullResultsBeacon,
		       'psFullResultsBeacon',
		       'Send Full Results to ');
};

// Do not install callbacks in unit tests because the callback holder will
// not exist.
if (PAGESPEED.PageSpeedContext) {

  // When the page speed panel is shown, update the export menu's status.
  PAGESPEED.PageSpeedContext.callbacks.showPageSpeed.addCallback(
      PAGESPEED.ResultsWriter.updateExportMenu);
}

})();  // End closure
