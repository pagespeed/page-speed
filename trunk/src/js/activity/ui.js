/**
 * Copyright 2008-2009 Google Inc.
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
 * @fileoverview Hooks called from various XUL UI components
 * declared in firebugOverlay.xul.
 * @author Bryan McQuade
 * @author David Bau
 */

goog.provide('activity.ui');

goog.require('activity.AppStateObserver');
goog.require('activity.Profiler');
goog.require('activity.TimelineManager');
goog.require('activity.fileutil');
goog.require('activity.preference');
goog.require('activity.xpcom');

/**
 * The xpcom contract id for the file picker.
 * @type {string}
 * @private
 */
activity.ui.FILE_PICKER_CONTRACT_ID_ = '@mozilla.org/filepicker;1';

/**
 * The xpcom interface name for the file picker.
 * @type {string}
 * @private
 */
activity.ui.FILE_PICKER_INTERFACE_NAME_ = 'nsIFilePicker';

/**
 * The xpcom contract id for the activityProfiler.
 * @type {string}
 * @private
 */
activity.ui.ACTIVITY_PROFILER_CONTRACT_ID_ =
    '@code.google.com/p/page-speed/ActivityProfiler;1';

/**
 * The xpcom interface name for the activityProfiler.
 * @type {string}
 * @private
 */
activity.ui.ACTIVITY_PROFILER_INTERFACE_NAME_ = 'IActivityProfiler';

/**
 * The preference name indicating whether or not we should collect
 * full call trees.
 * @type {string}
 * @private
 */
activity.ui.PREF_COLLECT_COMPLETE_CALL_GRAPHS_ = 'collect_complete_call_graphs';

/**
 * The preference name indicating whether or not we should profile JS.
 * @type {string}
 * @private
 */
activity.ui.PREF_ENABLE_JS_PROFILING_ = 'profile_js';

/**
 * The preference name to indicate whether data available events
 * should be instantaneous events, or they should show their full
 * start and end times.
 * @type {string}
 * @private
 */
activity.ui.PREF_INSTANTANEOUS_DATA_AVAILABLE_ =
    'ui_data_available_instantaneous';

/**
 * The xpcom contract id for the app info service.
 * @type {string}
 * @private
 */
activity.ui.XUL_RUNTIME_APP_INFO_CONTRACT_ID_ = '@mozilla.org/xre/app-info;1';

/**
 * The xpcom interface name for the nsIXULRuntime.
 * @type {string}
 * @private
 */
activity.ui.XUL_RUNTIME_INTERFACE_NAME_ = 'nsIXULRuntime';

/**
 * The xpcom contract id for the firebug service.
 * @type {string}
 * @private
 */
activity.ui.FIREBUG_CONTRACT_ID_ = '@joehewitt.com/firebug;1';

/**
 * Xpcom contract id for the observer service.
 * @type {string}
 * @private
 */
activity.ui.OBSERVER_SERVICE_CONTRACTID_ =
    '@mozilla.org/observer-service;1';

/**
 * Xpcom interface name for the observer service.
 * @type {string}
 * @private
 */
activity.ui.OBSERVER_SERVICE_INTERFACE_NAME_ = 'nsIObserverService';

/**
 * Heading displayed when there are incompatible add-ons installed.
 * @type {string}
 * @private
 */
activity.ui.INCOMPATIBLE_ADDONS_HEADING_ = 'Incompatible Add-ons';

/**
 * Message displayed when there are incompatible add-ons installed.
 * @type {string}
 * @private
 */
activity.ui.INCOMPATIBLE_ADDONS_MSG_ =
    'The following Firefox Add-ons are incompatible with Page Speed\'s ' +
    'Activity Panel and will cause some functionality to be disabled: ';

/**
 * Message displayed when there are incompatible add-ons installed.
 * @type {string}
 * @private
 */
activity.ui.DISABLE_INCOMPATIBLE_ADDONS_MSG_ =
    'To enable full functionality, please disable these Add-ons ' +
    '(Tools -> Add-ons, select the add-on, and choose "Disable"). ' +
    'When you are finished using Page Speed Activity, you can re-enable ' +
    'these Add-ons.';

/**
 * @param {string} name The name of the add-on.
 * @param {string} id the uuid of the add-on.
 * @constructor
 * @private
 */
activity.IncompatibleAddOnDescriptor_ = function(name, id) {
  /** @type {string} */
  this.name = name;

  /** @type {string} */
  this.id = id;
};

/**
 * @type {Array.<activity.IncompatibleAddOnDescriptor_>}
 * @private
 */
activity.ui.INCOMPATIBLE_ADDONS_ = [
    new activity.IncompatibleAddOnDescriptor_(
        'HttpWatch', '{1E2593B2-E106-4697-BCE7-A9D30DE05D73}')
    ];

/**
 * Invokes the callback function within a try/catch block. The catch
 * block invokes the specified exceptionHandler if the callback throws
 * an exception.
 * @param {Function} exceptionHandler the exception handler.
 * @param {Function} callbackFunction the function to invoke.
 * @param {Object} var_args Additional arguments that are applied to
 *     the callback function.
 * @private
 */
activity.ui.genericCallbackWrapper_ = function(
    exceptionHandler, callbackFunction, var_args) {
  try {
    var args = Array.prototype.slice.call(arguments, 2);
    callbackFunction.apply(null, args);
  } catch (e) {
    exceptionHandler(e);
  }
};

/**
 * Default exception handler for the activity profiler. Displays an
 * alert() with the stack trace.
 * @param {Object} exception the Exception to handle.
 * @private
 */
activity.ui.exceptionHandler_ = function(exception) {
  var msg;
  if (!('stack' in exception)) {
    msg = 'No stack trace available.\n' + exception;
  } else {
    msg = exception.stack;
  }
  alert('Page Speed Activity encountered an error:\n' + msg);
};

/**
 * Function that attempts to invoke the specified callback. If the
 * callback throws an exception, an alert() dialog is displayed.
 * @type {Function}
 * @private
 */
activity.ui.activityCallbackWrapper_ = goog.partial(
    activity.ui.genericCallbackWrapper_, activity.ui.exceptionHandler_);

/**
 * Set to true when the user is shown an alert saying their platform is
 * unsupported.  Used to avoid repeatedly showing this alert.
 * @private
 */
activity.ui.unsupportedAlertDone_ = false;

/**
 * Public entry point for invoking activity profiler UI commands.
 * @param {string} command The command to execute.
 * @private
 */
activity.ui.performCommand_ = function(command) {
  try {
    if (!activity.ui.isReady_()) {
      // Tell the user that their platform is unsupported once per window.
      // Without this test, each tab would fire an alert.
      if (!activity.ui.unsupportedAlertDone_) {
        alert(['Page Speed Activity is not currently supported on your ',
               'platform (',
               activity.ui.getPlatformName_(),
               ').'].join(''));
        activity.ui.unsupportedAlertDone_ = true;
      }
      return;
    }

    // Invoke the command, which is actually the name of a function.
    activity.ui[command]();
  } finally {
    activity.ui.updateUi_();
  }
};

/**
 * Function that invokes performCommand_ within the activity profiler callback
 * wrapper.
 * @type {Function}
 */
activity.ui.performCommand = goog.partial(
    activity.ui.activityCallbackWrapper_, activity.ui.performCommand_);

/**
 * Version of performCommand that can be attached to the onclick
 * handler.
 */
activity.ui.onClick = function(clickEvent, command) {
  if (clickEvent.button != 0) {
    // Only handle left-clicks.
    return;
  }
  if (clickEvent.eventPhase != 2) {
    // Don't handle clicks on child components.
    return;
  }
  activity.ui.performCommand(command);
};

/**
 * Builds the array of incompatible add-ons.
 * @return {Array.<string>} The array of incompatible add on names.
 * @private
 */
activity.ui.getIncompatibleAddOns_ = function() {
  var incompatibleAddOns = [];
  for (var i = 0, len = activity.ui.INCOMPATIBLE_ADDONS_.length; i < len; i++) {
    var addon = activity.ui.INCOMPATIBLE_ADDONS_[i];
    if (activity.ui.isAddonInstalledAndEnabled_(addon.id)) {
      incompatibleAddOns.push(addon.name);
    }
  }

  return incompatibleAddOns;
};

/**
 * Checks to see if any incompatible add-ons are installed, and adds
 * information about those add-ons to the infoPanel.
 * @param {nsIDOMElement} infoPanel the infoPanel XUL vbox element.
 * @private
 */
activity.ui.checkForIncompatibleAddOns_ = function(infoPanel) {
  var incompatibleAddOns = activity.ui.getIncompatibleAddOns_();

  if (incompatibleAddOns.length > 0) {
    var label = goog.global.document.createElement('label');
    var textNode = goog.global.document.createTextNode(
        activity.ui.INCOMPATIBLE_ADDONS_HEADING_);
    label.appendChild(textNode);
    label.setAttribute('class', 'bold');
    infoPanel.appendChild(label);

    label = goog.global.document.createElement('label');
    textNode = goog.global.document.createTextNode(
        activity.ui.INCOMPATIBLE_ADDONS_MSG_);
    label.appendChild(textNode);
    infoPanel.appendChild(label);

    label = goog.global.document.createElement('label');
    textNode = goog.global.document.createTextNode(
        incompatibleAddOns.join(', '));
    label.appendChild(textNode);
    infoPanel.appendChild(label);

    label = goog.global.document.createElement('label');
    label.setAttribute('value', '');
    infoPanel.appendChild(label);

    label = goog.global.document.createElement('label');
    textNode = goog.global.document.createTextNode(
        activity.ui.DISABLE_INCOMPATIBLE_ADDONS_MSG_);
    label.appendChild(textNode);
    infoPanel.appendChild(label);
  }
};

/**
 * Onload handler for the activity profiler timeline window.
 * @private
 */
activity.ui.timelineWindowOnLoad_ = function() {
  activity.ui.populateIncompatibleAddOnsView_();
  activity.ui.updateUi_();
};

/**
 * Populate the incompatible Add-ons view in the UI.
 * @private
 */
activity.ui.populateIncompatibleAddOnsView_ = function() {
  var infoPanel = activity.ui.getElementById_('activityInfo');
  // Remove old children.
  while (infoPanel.lastChild != null) {
    infoPanel.removeChild(infoPanel.lastChild);
  }
  activity.ui.checkForIncompatibleAddOns_(infoPanel);
};

/**
 * @param {nsIDOMWindow} win The window to render the activity profiler timeline
 *     within.
 */
activity.ui.setTimelineWindow = function(win) {
  if (win == activity.ui.timelineWindow_) return;
  activity.ui.stopProfiler();
  if (activity.ui.timelineWindow_) {
    // Update the old UI to reflect that the timeline has been
    // stopped.
    activity.ui.updateUi_();
  }
  activity.ui.timelineWindow_ = win;
  if (activity.ui.timelineWindow_) {
    // Make sure that we update the UI of the window once the onload
    // event fires.
    activity.ui.timelineWindow_.addEventListener(
        'load', activity.ui.timelineWindowOnLoad_, false);
  }
};

/**
 * @param {string} pref The name of the pref to toggle.
 * @private
 */
activity.ui.toggleBoolPref_ = function(pref) {
  activity.ui.reset();

  var oldValue = activity.preference.getBool(pref);
  activity.preference.setBool(pref, !oldValue);
};

/**
 * Toggle the "profile JS" pref.
 */
activity.ui.toggleProfileJavaScript = function() {
  activity.ui.toggleBoolPref_(activity.ui.PREF_ENABLE_JS_PROFILING_);
};

/**
 * Toggle the "complete call graphs" pref.
 */
activity.ui.toggleCompleteCallGraphs = function() {
  activity.ui.toggleBoolPref_(activity.ui.PREF_COLLECT_COMPLETE_CALL_GRAPHS_);
};

/**
 * Toggle the "instantaneous data available" pref.
 */
activity.ui.toggleInstantaneousDataAvailable = function() {
  activity.ui.toggleBoolPref_(activity.ui.PREF_INSTANTANEOUS_DATA_AVAILABLE_);
};

/**
 * Shut down services and tear down the UI.
 */
activity.ui.quitApplication = function() {
  activity.ui.reset();

  // Nothing more to do here. The actual UI will be updated in updateUi_(),
  // which is called from performCommand.
};

/**
 * Start profiler if not currently profiler, and stop profiler if currently
 * profiler.
 */
activity.ui.toggleProfiler = function() {
  if (activity.ui.isProfiling_) {
    activity.ui.stopProfiler();
  } else {
    activity.ui.startProfiler_();
  }
};

/**
 * Start the Activity profiler.
 * @private
 */
activity.ui.startProfiler_ = function() {
  activity.ui.reset();

  if (!activity.ui.timelineWindow_) return;

  activity.ui.isProfiling_ = true;

  activity.ui.collectFullCallTrees_ = activity.preference.getBool(
      activity.ui.PREF_COLLECT_COMPLETE_CALL_GRAPHS_, false);

  activity.ui.enableJsProfiling_ = activity.preference.getBool(
      activity.ui.PREF_ENABLE_JS_PROFILING_, true);

  activity.ui.appStateObserver_.register();

  activity.ui.activityProfiler_ = activity.xpcom.CCIN(
      activity.ui.ACTIVITY_PROFILER_CONTRACT_ID_,
      activity.ui.ACTIVITY_PROFILER_INTERFACE_NAME_);
  activity.ui.startTimeUsec_ =
      activity.ui.activityProfiler_.getCurrentTimeUsec();

  if (activity.ui.enableJsProfiling_) {
    activity.ui.firebugProtector_ = new activity.FirebugProtector(
        activity.ui.getFirebugServiceOrNull_(),
        activity.ui.firebugFunctionPatch_);
    activity.ui.firebugProtector_.start();
    activity.ui.profiler_.start();
    activity.ui.activityProfiler_.register(
        activity.ui.startTimeUsec_, activity.ui.collectFullCallTrees_);
  }

  var observerService = activity.xpcom.CCSV(
      activity.ui.OBSERVER_SERVICE_CONTRACTID_,
      activity.ui.OBSERVER_SERVICE_INTERFACE_NAME_);

  var xulRowsElement = activity.ui.instantiateUi_();
  activity.ui.timelineManager_.start(
      activity.ui.activityProfiler_,
      observerService,
      activity.ui.startTimeUsec_,
      activity.ui.timelineWindow_.document,
      xulRowsElement);
};

/**
 * Stop the Activity profiler.
 */
activity.ui.stopProfiler = function() {
  if (activity.ui.isProfiling_) {
    activity.ui.isProfiling_ = false;

    activity.ui.appStateObserver_.unregister();

    if (activity.ui.enableJsProfiling_) {
      activity.ui.activityProfiler_.unregister();
      activity.ui.profiler_.stop();
      activity.ui.firebugProtector_.stop();
    }

    activity.ui.timelineManager_.stop();
    activity.ui.firebugProtector_ = null;
    activity.ui.startTimeUsec_ = -1;
  }
};

/**
 * Save the current profile protocol buffer to a file on disk.
 */
activity.ui.save = function() {
  activity.ui.stopProfiler();

  var filePicker = activity.xpcom.CCIN(
      activity.ui.FILE_PICKER_CONTRACT_ID_,
      activity.ui.FILE_PICKER_INTERFACE_NAME_);
  filePicker.init(
      window, 'Save JavaScript Activity', filePicker.modeSave);
  filePicker.defaultString = 'activity.pb';
  filePicker.displayDirectory = activity.fileutil.getHomeDir();
  filePicker.appendFilter('Protocol buffer', '*.pb');
  filePicker.appendFilters(filePicker.filterAll);

  // Blocks until the user chooses a file.
  var result = filePicker.show();
  if (result == filePicker.returnOK || result == filePicker.returnReplace) {
    var targetFile = filePicker.file;
    try {
      activity.ui.activityProfiler_.dump(targetFile);
    } catch (e) {
      alert('Unable to save file ' + targetFile.path);
    }
  }
};

/**
 * Show the list of uncalled functions in a new window.
 */
activity.ui.showUncalledFunctions = function() {
  activity.ui.stopProfiler();

  var treeView = activity.ui.activityProfiler_.getUncalledFunctionsTreeView();
  if (treeView.rowCount == 0) {
    alert('No uncalled functions.');
    return;
  }
  var url = 'chrome://activity/content/uncalledFunctions.xul';
  activity.ui.openNewTreeViewWindow_(url, treeView);
};

/**
 * Show the list of delayable functions in a new window.
 */
activity.ui.showDelayableFunctions = function() {
  activity.ui.stopProfiler();

  var treeView = activity.ui.activityProfiler_.getDelayableFunctionsTreeView();
  if (treeView.rowCount == 0) {
    alert('No delayable functions.');
    return;
  }
  var url = 'chrome://activity/content/delayableFunctions.xul';
  activity.ui.openNewTreeViewWindow_(url, treeView);
};

/**
 * Open a new window with the given URL that renders the given tree view.
 * @param {string} url The URL.
 * @param {nsITreeView} treeView The tree view to bind to the XUL tree element.
 * @private
 */
activity.ui.openNewTreeViewWindow_ = function(url, treeView) {
  // Use the url as the unique window name.
  var windowName = url;

  var win = window.openDialog(url, windowName, 'chrome,dialog=no', treeView);
  win.focus();
};

/**
 * Tear down the UI and release the supporting data structures.
 */
activity.ui.reset = function() {
  activity.ui.stopProfiler();

  activity.ui.timelineManager_.reset();
  activity.ui.removeRows_();
  activity.ui.activityProfiler_ = null;
  activity.ui.firebugProtector_ = null;
};

/**
 * Update the activity profiler to reflect the state of the backing services.
 * @private
 */
activity.ui.updateUi_ = function() {
  var hasError = activity.ui.hasError_();
  if (hasError) {
    alert('Page Speed Activity has encountered an error.');
    // If there was an error, don't bother updating the state of the
    // rest of the UI.
    return;
  }

  // Create a short alias for getElementById_.
  var gebi = activity.ui.getElementById_;

  var activityContentPanel = gebi('activityContent');
  var activityProfileRecordButton = gebi('button-activityToggleProfiler');
  var activityProfileStopButton = gebi('button-activityStopProfiler');
  var activitySaveButton = gebi('button-activitySave');
  var activityShowUncalledButton = gebi('button-activityShowUncalled');
  var activityShowDelayableButton = gebi('button-activityShowDelayable');

  activityProfileRecordButton.checked = activity.ui.isProfiling_;
  activityProfileStopButton.disabled = !activity.ui.isProfiling_;

  var isProfileAvailable = activity.ui.activityProfilerHasProfile_();
  activitySaveButton.disabled = !isProfileAvailable;
  activityShowUncalledButton.disabled = !isProfileAvailable;
  activityShowDelayableButton.disabled = !isProfileAvailable;

  // If we aren't collecting full call trees, don't provide the UI
  // to save, or to view uncalled or delayable functions, since those
  // views are not accurate when using partial call trees.
  var showAdvancedJsButtons =
      activity.ui.enableJsProfiling_ && activity.ui.collectFullCallTrees_;

  // Disable saving on Windows, until we fix the Windows-only crash
  // in protobuf SerializeToFileDescriptor().
  activitySaveButton.collapsed = activity.ui.isOsWindows_() ||
      !activity.ui.enableJsProfiling_ || !showAdvancedJsButtons;

  activityShowUncalledButton.collapsed = !showAdvancedJsButtons;
  activityShowDelayableButton.collapsed = !showAdvancedJsButtons;

  if (activityContentPanel) {
    activityContentPanel.selectedIndex = isProfileAvailable ? 1 : 0;
  }
};

/**
 * Construct the UI.
 * @return {nsIDOMElement} the XUL rows element to build the timeline under.
 * @private
 */
activity.ui.instantiateUi_ = function() {
  var grid = activity.ui.getElementById_('activityGrid');

  // First time through: extract the rows template from the DOM, and remove
  // it. Keep a reference to it so we can clone it each time we want to
  // render a new timeline.
  var rowsTemplate = activity.ui.getElementById_('activity-rowsTemplate');
  if (rowsTemplate) {
    rowsTemplate.parentNode.removeChild(rowsTemplate);
    activity.ui.rowsTemplate_ = rowsTemplate;
  }

  // Create a deep copy of the rows template.
  var rows = activity.ui.rowsTemplate_.cloneNode(true);
  rows.setAttribute('collapsed', false);
  grid.appendChild(rows);
  return rows;
};

/**
 * Remove all Rows under the grid view.
 * @private
 */
activity.ui.removeRows_ = function() {
  /**
   * The root XUL dom element for our grid.
   * @type {!nsIDOMElement}
   */
  var xulGridElement = activity.ui.getElementById_('activityGrid');

  // Remove the previous rows, if any.
  if (xulGridElement && xulGridElement.hasChildNodes()) {
    var children = xulGridElement.childNodes;
    for (var i = children.length - 1; i >= 0; i--) {
      if (children[i].tagName.toLowerCase() == 'rows') {
        xulGridElement.removeChild(children[i]);
      }
    }
  }
};

/**
 * @return {boolean} whether we've encountered an error.
 * @private
 */
activity.ui.hasError_ = function() {
  return activity.ui.activityProfiler_ &&
      activity.ui.activityProfiler_.hasError();
};

/**
 * @return {boolean} whether the activityProfiler has a profile.
 * @private
 */
activity.ui.activityProfilerHasProfile_ = function() {
  if (activity.ui.timelineManager_ == null || activity.ui.hasError_()) {
    return false;
  }

  var state = activity.ui.timelineManager_.getState();
  return state == activity.TimelineManager.State.STARTED ||
         state == activity.TimelineManager.State.FINISHED;
};

/**
 * @return {boolean} whether or not the component we depend on is loaded.
 * @private
 */
activity.ui.isReady_ = function() {
  return activity.xpcom.CC(activity.ui.ACTIVITY_PROFILER_CONTRACT_ID_) != null;
};

/**
 * @return {string} the name of the current platform.
 * @private
 */
activity.ui.getPlatformName_ = function() {
  var xulRuntime = activity.xpcom.CCSV(
    activity.ui.XUL_RUNTIME_APP_INFO_CONTRACT_ID_,
    activity.ui.XUL_RUNTIME_INTERFACE_NAME_);
  return xulRuntime.OS + '_' + xulRuntime.XPCOMABI;
};

/**
 * @return {boolean} whether the current OS is windows.
 * @private
 */
activity.ui.isOsWindows_ = function() {
  var xulRuntime = activity.xpcom.CCSV(
    activity.ui.XUL_RUNTIME_APP_INFO_CONTRACT_ID_,
    activity.ui.XUL_RUNTIME_INTERFACE_NAME_);
  return (xulRuntime.OS == 'WINNT');
};

/**
 * @param {string} name The name of the DOM element.
 * @return {nsIDOMElement} the XUL DOM element with the given id.
 * @private
 */
activity.ui.getElementById_ = function(name) {
  var element;
  if (activity.ui.timelineWindow_ && activity.ui.timelineWindow_.document) {
    // First try to get the element from the timeline window.
    element = activity.ui.timelineWindow_.document.getElementById(name);
  }
  if (!element) {
    // If we're unable to find it in the timeline window, see if it's
    // a member of the main window.
    element = goog.global.document.getElementById(name);
  }

  return element;
};

/**
 * The function to patch over the non-whitelisted Firebug service functions.
 * @private
 */
activity.ui.firebugFunctionPatch_ = function() {
  if (!activity.ui.firebugFunctionPatchInvoked_) {
    // Only show the alert dialog once per session.
    activity.ui.firebugFunctionPatchInvoked_ = true;
    alert(
        'Page Speed Activity intercepted a Firebug service call. '
        + 'Please file a bug at '
        + 'http://code.google.com/p/page-speed/issues/entry\n'
        + new Error().stack);
  }
};

/**
 * Get the FirebugService object, or null if Firebug is not installed.
 * @return {Object} firebug service singleton.
 * @private
 */
activity.ui.getFirebugServiceOrNull_ = function() {
  var firebugClass = Components.classes[activity.ui.FIREBUG_CONTRACT_ID_];
  if (firebugClass) {
    /** @type {FirebugService} */
    var service = firebugClass.getService();
    return service.wrappedJSObject;
  } else {
    return null;
  }
};

/**
 * Helper that checks to see if the given attribute is set to true for
 * the given RDF resource.
 * @param {nsIRDFDataSource} dataSource the RDF data source.
 * @param {nsIRDFService} rdfService the RDF service.
 * @param {nsIRDFResource} addon the resource for the given add-on.
 * @param {string} attribute the attribute to check.
 * @return {boolean} whether or not the given attribute is set to true
 *     for the given resource.
 * @private
 */
activity.ui.isResourceAttributeTrue_ = function(
    dataSource, rdfService, addon, attribute) {
  var appRes = rdfService.GetResource(
      'http://www.mozilla.org/2004/em-rdf#' + attribute);
  var target = dataSource.GetTarget(addon, appRes, true);
  if (!target) {
    return false;
  }

  target = activity.xpcom.QI(target, activity.xpcom.CI('nsIRDFLiteral'));
  return (target.Value == 'true');
};

/**
 * @param {string} uuid the uuid of the add-on to check.
 * @return {boolean} whether or not the add-on with the given uuid is
 *     installed and enabled.
 * @private
 */
activity.ui.isAddonInstalledAndEnabled_ = function(uuid) {
  // based on firefox code
  var rdfService = activity.xpcom.CCSV(
      '@mozilla.org/rdf/rdf-service;1', 'nsIRDFService');
  var extensionManager = activity.xpcom.CCSV(
      '@mozilla.org/extensions/manager;1', 'nsIExtensionManager');
  if (!extensionManager.getItemForID(uuid)) {
    // not installed.
    return false;
  }
  var dataSource = extensionManager.datasource;
  var addon = rdfService.GetResource('urn:mozilla:item:' + uuid);
  if (!addon) {
    return false;
  }

  if (activity.ui.isResourceAttributeTrue_(
          dataSource, rdfService, addon, 'appDisabled') ||
      activity.ui.isResourceAttributeTrue_(
          dataSource, rdfService, addon, 'userDisabled')) {
    // add-on is installed, but disabled.
    return false;
  }


  return true;
};

/**
 * Xpconnect handle to the native Activity profiler.
 * @type {IActivityProfiler?}
 * @private
 */
activity.ui.activityProfiler_ = null;

/**
 * AppStateObserver instance. Listens for shutdown events.
 * @type {activity.AppStateObserver}
 * @private
 */
activity.ui.appStateObserver_ = new activity.AppStateObserver();

/**
 * The JSD Profiler instance.
 * @type {activity.Profiler}
 * @private
 */
activity.ui.profiler_ = new activity.Profiler();

/**
 * The firebug protector instance.
 * @type {activity.FirebugProtector}
 * @private
 */
activity.ui.firebugProtector_ = null;

/**
 * The TimelineManager instance.
 * @type {activity.TimelineManager}
 * @private
 */
activity.ui.timelineManager_ = new activity.TimelineManager(
    goog.global, activity.ui.activityCallbackWrapper_);

/**
 * A XUL rows element that contains a template of the activity
 * profiler timeline.
 * @type {nsIDOMElement}
 * @private
 */
activity.ui.rowsTemplate_ = null;

/**
 * Whether or not activity profiler is profiling.
 * @type {boolean}
 * @private
 */
activity.ui.isProfiling_ = false;

/**
 * Whether or not the firebug function patch has been invoked.
 * @type {boolean}
 * @private
 */
activity.ui.firebugFunctionPatchInvoked_ = false;

/**
 * The start time of the currently active profiling session, or -1 if
 * no profiling session is active.
 * @type {number}
 * @private
 */
activity.ui.startTimeUsec_ = -1;

/**
 * Whether or not to collect full call trees. Collecting full call
 * trees gives more information, but is more expensive, and introduces
 * the "observer effect".
 * @type {boolean}
 * @private
 */
activity.ui.collectFullCallTrees_ = false;

/**
 * Whether or not to profile JavaScript.
 * @type {boolean}
 * @private
 */
activity.ui.enableJsProfiling_ = true;

/**
 * The window to render the activity profiler timeline within.
 * @type {nsIDOMWindow}
 * @private
 */
activity.ui.timelineWindow_ = null;
