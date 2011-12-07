/**
 * Copyright 2007-2009 Google Inc.
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
 * @fileoverview Integrates Page Speed with Firebug.
 *
 * To create a new lint rule, create a new LintRule instance in a separate .js
 * file, then register the function with PAGESPEED.LintRules.registerLintRule()
 *
 * @author Kyle Scholz
 */

(function() {  // Begin closure

Components.utils.import("resource://gre/modules/ctypes.jsm");

/**
 * PAGESPEED.PageSpeedContext is a Singleton within each window.
 * A window may contain many tabs, and there is a single PageSpeedContext
 * for all tabs in a window.
 * @constructor
 */
PAGESPEED.PageSpeedContext = function() {
  this.callbacks = {
    // Run when firebug calls PageSpeedModule.initialize().
    initPageSpeedModule: new PAGESPEED.CallbackHolder(),

    // Run after PAGESPEED.PageSpeedContext.displayPerformance() is complete.
    postDisplay: new PAGESPEED.CallbackHolder(),

    // Run after Firebug.PageSpeedModule.watchWindow() is complete on
    // the top level window object (as opposed to a frame or iframe).
    watchWindow: new PAGESPEED.CallbackHolder(),

    // Run after Firebug.PageSpeedModule.unwatchWindow() is complete on the top
    // level window object.
    unwatchWindow: new PAGESPEED.CallbackHolder(),

    // Run when the pagespeed tab is shown in firebug.
    showPageSpeed: new PAGESPEED.CallbackHolder(),

    // Run right before we invoke the rules to generate a score. All
    // callees must invoke the 'callback' property of the callback
    // argument once they are done performing any operations that need
    // to run before the scoring code runs. Failure to invoke the
    // callback will cause Page Speed to hang, since it waits until all
    // callbacks have been invoked before proceeding.
    beforeScoring: new PAGESPEED.CallbackHolder(),

    // Called when PageSpeed is considering refetching a url.  If ANY
    // callback installed returns true, then the URL will NOT be fetched.
    isRefetchForbidden: new PAGESPEED.CallbackHolder()
  };
};

/**
 * Given a function name and an exception, log an error message.
 * @param {string} functionName Name of the function where the
 *     exception was caught.
 * @param {Error} e The exception raised.
 */
function logException(functionName, e) {
  PS_LOG(['Uncaught exception in function ', functionName, '.  \n',
          'Please file a bug with the following text: ',
          PAGESPEED.Utils.formatException(e)].join(''));
}

/**
 * Displays the score card.
 * @param {Object} panel Firebug-created object that stores state of the
 *     the panel where results are displayed.
 * @param {Object} browserTab The browser object of the tab PageSpeed is
 *     running on.
 */
PAGESPEED.PageSpeedContext.prototype.displayPerformance = function(
    panel, browserTab) {

  if (PAGESPEED.LintRules.nativeRuleResults.length == 0) {
    // If we failed to generate any native rules, it might be because
    // the user's browser or OS is not compatible with our native
    // library. If we can't instantiate the native library
    // then show them the incompatible browser error message.
    var openedNativeLibrary = false;
    try {
      var lib = ctypes.open(PAGESPEED.Utils.getNativeLibraryPath());
      if (lib) {
        openedNativeLibrary = true;
        lib.close();
      }
    } catch (e) {
    }
    if (!openedNativeLibrary) {
      panel.table = panel.incompatibleBrowserPageTag.replace(
          {}, panel.panelNode, panel);
    } else {
      // We encountered an error running the native rules so we should
      // show an error page.
      var docUrl = 'current page';
      if (Firebug.currentContext.window &&
          Firebug.currentContext.window.document &&
          Firebug.currentContext.window.document.URL) {
        docUrl = Firebug.currentContext.window.document.URL;
      }
      panel.table = panel.errorPageTag.replace(
          {'currentPageUrl': docUrl}, panel.panelNode, panel);
    }
    return;
  }

  // The rules array holds the rule objects used to fill the domplate.
  var rules = [];
  var lintRules = PAGESPEED.LintRules.lintRules.concat(
    PAGESPEED.LintRules.nativeRuleResults);

  // Assigne undefined rule impact to -1, so that it can compare.
  for (var i = 0, len = lintRules.length; i < len; i++) {
    if (typeof lintRules[i].rule_impact == 'undefined') {
      lintRules[i].rule_impact = -1;
    }
  }

  var compare = function (a, b) {
    return a < b ? -1 : a > b ? 1 : 0;
  };

  // Sort the rule results, first by impact (descending), then by number of
  // results (descending), then by name (ascending).  Sorting by number of
  // results is in there so that zero-impact rules with results come before
  // zero-impact rules with no results.
  lintRules.sort(function (result1, result2) {
      var res = compare(result1.experimental, result2.experimental);
      if (res !== 0) return res;
      res = compare(result2.rule_impact, result1.rule_impact);
      if (res !== 0) return res;
      res = compare((result2.url_blocks || []).length,
                    (result1.url_blocks || []).length);
      if (res !== 0) return res;
      return compare(result1.name, result2.name);
  });

  // Given a score, return a red/yellow/green color.
  var makeScoreColor = function (score) {
    if (score === 'disabled') {
      return PAGESPEED.Utils.SCORE_CODE_INFO;
    }
    return (score > 80 ? PAGESPEED.Utils.SCORE_CODE_GREEN :
            score > 60 ? PAGESPEED.Utils.SCORE_CODE_YELLOW :
            PAGESPEED.Utils.SCORE_CODE_RED);
  };

  for (var i = 0, len = lintRules.length; i < len; i++) {
    rules.push(panel.createRuleTagDomplateData(lintRules[i]));
  }

  var overallScore = PAGESPEED.LintRules.score;
  var overallScoreColorCode = makeScoreColor(overallScore);

  panel.table = panel.tableTag.replace(
      panel.createTableTagDomplateData(overallScoreColorCode, overallScore),
      panel.panelNode, panel);

  var tbody = panel.table.firstChild;
  var lastRow = tbody.lastChild;
  var row = panel.ruleTag.insertRows({'rules': rules}, lastRow)[0];
  for (var i = 0; i < rules.length; ++i) {
    row.repObject = rules[i];
    rules[i].row = row;
    row = row.nextSibling;
  }
  return overallScore;
}

/**
 * Displays the score card and continue going through state machine
 * of analyzing, displaying and reporting new results.
 *
 * @param {Object} panel Firebug-created object that stores state of the
 *     the panel where results are displayed.
 * @param {Object} browserTab The browser object of the tab PageSpeed is
 *     running on.
 */
PAGESPEED.PageSpeedContext.prototype.processResults = function(
    panel, browserTab) {

  // Display the results
  var overallScore = PAGESPEED.PageSpeedContext.displayPerformance(
      panel, browserTab);

  // Build the results object.
  var resultsContainer = new PAGESPEED.ResultsContainer(
      browserTab, overallScore);

  // Install the results container on the current tab, so that other code can
  // fetch it.
  PAGESPEED.ResultsContainer.addResultsContainerToTab(
      browserTab, resultsContainer);

  // Pass the browser object that holds data for the current tab.
  // There is only one callback holder per window (which can contain
  // many tabs).  Passing in a reference to the tab allows tab
  // specific information (such as the page load time) to be found.
  this.callbacks.postDisplay.execCallbacks(
      {
        overallScore: overallScore,
        browserTab: browserTab,
        resultsContainer: resultsContainer
      });

  // When automating firefox, it is handy to be able to make firefox
  // quit after computing scores.
  // This pref is somewhat dangerous, because a user who sets it by
  // mistake, and has 'run at onload' enabled, will see firefox quit
  // every time a page is loaded.  Leave this setting out of the GUI,
  // so that only those users who know what they are doing will set it.
  if (PAGESPEED.Utils.getBoolPref(
          'extensions.PageSpeed.quit_after_scoring', false)) {
    var quitFirefoxDelay = PAGESPEED.Utils.getIntPref(
        'extensions.PageSpeed.quit_after_scoring_delay', 0);
    if (!quitFirefoxDelay) {
      PAGESPEED.Utils.quitFirefox();
    } else {
      setTimeout(PAGESPEED.Utils.quitFirefox, quitFirefoxDelay);
    }
  }
};

/**
 * Displays the list of components for the current window.
 */
PAGESPEED.PageSpeedContext.prototype.displayComponents = function(panel) {
  var comps = [];
  var totalComponents = 0;
  var totalFileSize = 0;
  var totalTransferSize = 0;
  var urls = PAGESPEED.Utils.getResources();

  for (var i = 0, len = urls.length; i < len; ++i) {
    var url = urls[i];
    var fileSize = PAGESPEED.Utils.getResourceSize(url);
    var transferSize = PAGESPEED.Utils.getResourceTransferSize(url);
    var cachedStr = PAGESPEED.Utils.getFromCache(url) ? '(cache)' : '';
    ++totalComponents;
    totalFileSize += fileSize;
    totalTransferSize += transferSize;

    comps.push({
                 'type': PAGESPEED.Utils.getResourceTypes(url).join(','),
                 'href': url,
                 'path': PAGESPEED.Utils.getDisplayUrl(url),
                 'statusCode': PAGESPEED.Utils.getResponseCode(url),
                 'fromCache': cachedStr,
                 'domain': PAGESPEED.Utils.getDomainFromUrl(url),
                 'filesize': PAGESPEED.Utils.formatBytes(fileSize),
                 'xfersize': PAGESPEED.Utils.formatBytes(transferSize)
    });
  }

  panel.table = panel.componentsTag.replace({
      'totalComponents': totalComponents + ' resources',
      'totalTransferSize': PAGESPEED.Utils.formatBytes(totalTransferSize),
      'totalFileSize': PAGESPEED.Utils.formatBytes(totalFileSize)},
      panel.panelNode, panel);

  var tbody = panel.table.firstChild;
  var lastRow = tbody.lastChild.previousSibling;
  var row = panel.componentTag.insertRows({'components': comps},
                                          lastRow)[0];

  // TODO: This weird boiler plate code for domplate was copied from
  // Firebug and is duplicated in displayPerformance. We should determine if
  // it can be simplified and at minimum factor out the duplication.
  for (var i = 0; i < comps.length; ++i) {
    row.repObject = comps[i];
    comps[i].row = row;
    row = row.nextSibling;
  }
};

/**
 * Helper that calls the function with name 'name' on each rule,
 * passing args as the set of args to that function. Used to call
 * initContext and destroyContext on rules, to allow them to hook
 * into these (and possibly other) firebug events.
 */
PAGESPEED.PageSpeedContext.prototype.dispatchToRules = function(name, args) {
  var lintRules = PAGESPEED.LintRules.lintRules;
  var rule;
  for (var i = 0, len = lintRules.length; i < len; ++i) {
    rule = lintRules[i];
    if (name in rule) {
      rule[name].apply(rule, args);
    }
  }
};


// Set up PageSpeedContext as a Singleton
PAGESPEED.PageSpeedContext = new PAGESPEED.PageSpeedContext();


/**
 * If Page Speed is unhealthy, stop initializing here.
 */
PAGESPEED.isHealthy && PAGESPEED.Utils.checkNamespaceDependencies();
if (!PAGESPEED.isHealthy) return;

FBL.ns(function() { with (FBL) {
/**
 * Create a Firebug Module so that we can use Firebug Hooks.
 * @extends {Firebug.Module}
 */
Firebug.PageSpeedModule = extend(Firebug.Module, {
  /**
   * Called when the window is opened.
   * @override
   */
  initialize: function() {
    try {
      PAGESPEED.PageSpeedContext.callbacks.initPageSpeedModule.execCallbacks(
          {PageSpeedModule: this});
    } catch (e) {
      logException('PageSpeedModule.initialize()', e);
    }
  },

  /**
   * Called when a new context is created but before the page is loaded.
   * @override
   */
  initContext: function(context) {
    try {
      // Invoke 'initContext' on any rules that implement that method.
      PAGESPEED.PageSpeedContext.dispatchToRules('initContext', [context]);
    } catch (e) {
      logException('PageSpeedModule.initContext()', e);
    }
  },

  /**
   * Called when a context is destroyed.
   * @override
   */
  destroyContext: function(context) {
    try {
      // Invoke 'destroyContext' on any rules that implement that method.
      PAGESPEED.PageSpeedContext.dispatchToRules('destroyContext', [context]);
    } catch (e) {
      logException('PageSpeedModule.destroyContext()', e);
    }
  },

  /** @override */
  showPanel: function(browser, panel) {
    try {
      var isPageSpeed = panel && 'pagespeed' == panel.name;
      // The browser.chrome may not be available when Page Speed is first loaded
      // in Firefox.
      if (!browser.chrome) {
        return;
      }

      var pagespeedButtons = browser.chrome.$('fbPageSpeedButtons');
      FBL.collapse(pagespeedButtons, !isPageSpeed);

      if (isPageSpeed) {
        PAGESPEED.PageSpeedContext.callbacks.showPageSpeed.execCallbacks({
           browserTab: PAGESPEED.Utils.getGBrowser().selectedBrowser
        });
      }
    } catch (e) {
      logException('PageSpeedModule.showPanel()', e);
    }
  },

  showPerformance: function() {
    var panel = Firebug.currentContext.getPanel('pagespeed');
    var browserTab = PAGESPEED.Utils.getGBrowser().selectedBrowser;

    // Only if we are still displaying the same page as the most
    // recently computed rules will we redisplay the panel.  Otherwise
    // put up the default panel with a button to analyze results
    var currentURI = browserTab.currentURI;

    if (currentURI &&
        currentURI.spec &&
        (currentURI.spec == PAGESPEED.LintRules.url)) {
      PAGESPEED.PageSpeedContext.displayPerformance(
          panel,
          browserTab);
    } else {
      panel.initializeNode(panel);
    }
    document.getElementById('psContentAnalyzeMenu').setAttribute(
        'collapsed', false);
  },

  analyzePerformance: function() {
    try {
      PAGESPEED.LintRules.stop();

      // Get the object that represents the current tab.
      var browserOfCurrentTab = PAGESPEED.Utils.getGBrowser().selectedBrowser;

      // Keep track of the number of outstanding callbacks. Once the
      // number goes to zero, we can start the scoring process.
      var numBeforeScoringCallbacks =
          PAGESPEED.PageSpeedContext.callbacks.beforeScoring.getNumCallbacks();

      if (numBeforeScoringCallbacks > 0) {
        var clientCallback = function() {
          numBeforeScoringCallbacks--;
          if (numBeforeScoringCallbacks == 0) {
            // All of the before scoring clients are finished, so we
            // can now start the scoring process.
            PAGESPEED.LintRules.exec(browserOfCurrentTab);
          }
        };

        // Tell each client that we're about to start scoring.
        PAGESPEED.PageSpeedContext.callbacks.beforeScoring.execCallbacks(
          {
            callback: clientCallback
          });
      } else {
        // No before scoring callbacks, so we can just invoke the
        // rules directly.
        PAGESPEED.LintRules.exec(browserOfCurrentTab);
      }
    } catch (e) {
      logException('PageSpeedModule.analyzePerformance()', e);
    }
  },

  showComponents: function() {
    try {
      PAGESPEED.LintRules.stop();

      document.getElementById('psContentAnalyzeMenu').setAttribute(
          'collapsed', true);

      PAGESPEED.PageSpeedContext.displayComponents(
          Firebug.currentContext.getPanel('pagespeed'));

    } catch (e) {
      logException('PageSpeedModule.showComponents()', e);
    }
  },

  exportMenuItemChosen: function(menuItem) {
    try {
      // Get the object that represents the current tab.
      var browserOfCurrentTab = PAGESPEED.Utils.getGBrowser().selectedBrowser;
      var resultsContainer =
          PAGESPEED.ResultsContainer.getResultsContainerByTab(
              browserOfCurrentTab);

      switch (menuItem) {
        case 'psJsonExport':
          PAGESPEED.ResultsWriter.openJsonExportDialog(resultsContainer);
          break;

        case 'psMinimalBeacon':
          var beaconDomain = PAGESPEED.minimalBeacon.getBeaconDomain();
          var title = ['Send score to ', beaconDomain, '?'].join('');
          var message = [
            'Sending scores to ', beaconDomain, ' allows that website, ',
            'which is not affiliated with Google, to collect and analyze ',
            'Page Speed scores for many websites. If you choose to send ',
            'scores to ', beaconDomain, ', your IP address, the URL you run ',
            'Page Speed on, and the scores Page Speed returns will be sent ',
            'to ', beaconDomain, ' for public display.'].join('');
          if (PAGESPEED.Utils.promptConfirm(title, message)) {
            var resultMessage;
            if (PAGESPEED.minimalBeacon.sendBeacon(resultsContainer)) {
              resultMessage = ['Scores sent to ', beaconDomain, '.'].join('');
            } else {
              resultMessage = [
                  'Failed to send scores to ', beaconDomain, '.'].join('');
            }
            alert(resultMessage);
          }
          break;

        case 'psFullResultsBeacon':
          PAGESPEED.fullResultsBeacon.sendBeacon(resultsContainer);
          break;

        default:
          PS_LOG('Unknown menu item "' + menuItem + '".');
      }

    } catch (e) {
      logException('PageSpeedModule.exportMenuItemChosen(' + menuItem + ')', e);
    }
  },

  openJsonExportDialog: function() {
    // Get the object that represents the current tab.
    var browserOfCurrentTab = PAGESPEED.Utils.getGBrowser().selectedBrowser;

    PAGESPEED.ResultsWriter.openJsonExportDialog(browserOfCurrentTab);
  },

  showHelp: function() {
    try {
      PAGESPEED.Utils.openLink(
        'http://code.google.com/speed/page-speed/docs/using.html');
    } catch (e) {
      logException('PageSpeedModule.showHelp()', e);
    }
  },

  reportIssues: function() {
    try {
      PAGESPEED.Utils.openLink('http://code.google.com/p/page-speed/issues/');
    } catch (e) {
      logException('PageSpeedModule.reportIssues()', e);
    }
  },

  pagespeedOnload: function(win) {
    try {
      // Record a timestamp at onload.  A timestamp is also added to each
      // resource when it is first fetched in the component collector.
      // This allows rules to figure out which resources are loaded before
      // onload.
      win.onloadTime = (new Date()).getTime();

      // If the autorun pref is set, then run Page Speed rules.
      if (PAGESPEED.Utils.getBoolPref('extensions.PageSpeed.autorun')) {
        var doc = win.document;
        try {
          if (!doc || !doc.location || !doc.location.hostname)
            return;
        } catch (e) {
          // Accessing doc.location.hostname can throw NS_ERROR_FAILURE
          // in firefox 3.5.0.  This error means there is no host,
          // so return;
          return;
        }

        // Want any javascript in the page which runs at onload to run before
        // Page Speed starts.  To allow this, launch pagespeed after a delay.
        // The default is 100ms.  Users might want to set a longer delay to be
        // sure all javascript has run.  This can be done with the preference
        // extensions.PageSpeed.autorun.delay.
        var delay = PAGESPEED.Utils.getIntPref(
            'extensions.PageSpeed.autorun.delay', 100);
        win.setTimeout(Firebug.PageSpeedModule.analyzePerformance, delay);
      }
    } catch (e) {
      logException('PageSpeedModule.pagespeedOnload()', e);
    }
  },

  showContext: function (browser, context) {
    // Firebug calls this when we change browser tabs (and at some other times
    // too).  We use the opportunity to make sure we're correctly displaying
    // the Performance or Resource pane, depending on what has been selected.
    if (document.getElementById('psCompButton').getAttribute('checked')) {
      Firebug.PageSpeedModule.showComponents();
    } else {
      Firebug.PageSpeedModule.showPerformance();
    }
  },

  /**
   * When a window is loaded, track redirects and add an event listener for
   * page loads.
   *
   * @param {Object} context The Firebug context linked to the window.
   * @param {Object} win The window object assocated with the tab/window
   *     being loaded.
   */
  watchWindow: function(context, win) {
    try {
      // watchWindow is called on iframes as well as top level windows.  Only
      // run Page Speed when the onload event for the top level window fires.
      if (win == win.top) {
        PAGESPEED.LintRules.stop();
        PAGESPEED.PageSpeedContext.callbacks.watchWindow.execCallbacks(
          {
            window: win,
            context: context
          });

        var onloadFn = function() {
          Firebug.PageSpeedModule.pagespeedOnload(win);
        };
        win.addEventListener('load', onloadFn, false);
      }
    } catch (e) {
      logException('PageSpeedModule.watchWindow()', e);
    }
  },

  /**
   * When a window is unloaded, remove the pagespeed object.
   * @param {Object} context The Firebug context linked to the window.
   * @param {Object} win The window object assocated with the tab/window
   *     being loaded.
   */
  unwatchWindow: function(context, win) {
    try {
      if (win == win.top) {
        PAGESPEED.PageSpeedContext.callbacks.unwatchWindow.execCallbacks(
          {
            window: win,
            browserTab: PAGESPEED.Utils.getGBrowser().selectedBrowser
          });
      }
    } catch (e) {
      logException('PageSpeedModule.unwatchWindow()', e);
    }
  }
});  // PageSpeedModule

Firebug.registerModule(Firebug.PageSpeedModule);

}});  // FBL.ns

})();  // End closure
