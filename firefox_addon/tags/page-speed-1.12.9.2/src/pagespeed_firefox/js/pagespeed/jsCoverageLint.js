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
 * @fileoverview Rule to determine js coverage.
 *
 * @author Bryan McQuade
 */

(function() {  // Begin closure

var nsIWebProgressListener = Components.interfaces.nsIWebProgressListener;
var nsIWebProgress = Components.interfaces.nsIWebProgress;
var nsISupportsWeakReference = Components.interfaces.nsISupportsWeakReference;
var nsISupports = Components.interfaces.nsISupports;
var NOTIFY_STATE_WINDOW = nsIWebProgress.NOTIFY_STATE_WINDOW;
var STATE_STOP = nsIWebProgressListener.STATE_STOP;
var STATE_IS_WINDOW = nsIWebProgressListener.STATE_IS_WINDOW;
var URL_JS_COVERAGE_WIKI =
    'http://code.google.com/speed/page-speed/docs/using.html#JSCoverageTest';
var MIN_UNCALLED_FUNCTIONS = 25;
var MAX_FUNCTIONS_PER_SCRIPT_PREF =
    'extensions.PageSpeed.js_coverage.max_functions_per_script';
var MAX_FUNCTIONS_PER_SCRIPT_DEFAULT = 100;
var SCRIPT_CACHE_NAME = 'scriptCache';
var JS_PROFILER_ENABLE_PREF = 'extensions.PageSpeed.js_coverage.enable';
var TRACK_FUNCTION_SIZES_PREF =
        'extensions.PageSpeed.js_coverage.track_function_size';
var TRACK_FUNCTION_SIZES_DEFAULT = false;

/**
 * @this PAGESPEED.LintRule
 */
var jsCoverageRule = function() {
  if (PAGESPEED.Utils.isUsingFilter()) {
    this.score = 'disabled';
    this.warnings = '';
    this.information =
      ['This rule is not currently able to filter results ',
       'and is not included in the analysis.'].join('');
    return;
  }

  if (!PAGESPEED.Utils.getBoolPref(JS_PROFILER_ENABLE_PREF)) {
    this.score = 'disabled';
    this.warnings = '';
    this.information =
      ['JavaScript function profiling is currently disabled. ',
       'To enable, make sure \"Profile Deferrable JavaScript\" is checked ',
       'in the Options menu.'
       ].join('');
    return;
  }

  if (!isFirebugApiAvailable()) {
    this.score = 'error';
    this.warnings = '';
    this.information =
      ['The Firebug JavaScript profiling API is unavailable. ',
       'Please use a version of Firebug known to be compatible with Page Speed.'
       ].join('');
    return;
  }

  // Fetch the context object for the script panel.
  var context = PAGESPEED.Utils.getPageSpeedContext();
  var jsCoverageContext = context.jsCoverageContext;

  if (jsCoverageContext == null) {
    this.score = 'error';
    this.warnings = '';
    this.information = 'Unable to profile JavaScript.';
    return;
  }

  if (jsCoverageContext.profiler &&
      jsCoverageContext.profiler.isProfiling()) {
    // The profiler is collecting profile data, but we haven't
    // inspected that data yet. Do so now.
    jsCoverageContext.profiler.profileScripts();
    jsCoverageContext.profiler.stopProfiling();
  }

  // If we have a profiler, then we're the ones actually profiling
  // this page, so it is not cached. If there is no profiler, then
  // some other context previously generated this result, so it is
  // cached.
  var cached = (jsCoverageContext.profiler == null);

  if (!cached) {
    // Clean up the results object the first time we process
    // it. Subsequent views of the cached results do not need to
    // re-clean the results object.
    cleanUpResults(jsCoverageContext.result, context);
  }

  var result = jsCoverageContext.result;

  // See if profiling failed in some way. If so, getErrorTextOrNull
  // returns the error text. If no failure, getErrorTextOrNull returns
  // null.
  var errorText = getErrorTextOrNull(result);
  if (errorText) {
    this.score = 'error';
    this.warnings = '';
    this.information = errorText;
    return;
  }

  // If we've made it this far, then we have a valid result
  // object. We'll use it to compute a score and present information
  // to the user.
  var uncalledCountOrSize = result.uncalledCountOrSize;
  if (uncalledCountOrSize < MIN_UNCALLED_FUNCTIONS) {
    // Too few functions on this page for this rule to make a
    // significant latency impact.
    this.score = 'n/a';
    return;
  }

  var totalCountOrSize = result.totalCountOrSize;
  var percentageUncalled = uncalledCountOrSize / totalCountOrSize;

  this.score = getScoreForPercentage(1 - percentageUncalled);

  // Generate a summary list of all of the page components that
  // included uncalled functions, as well as the definitions of
  // those functions.
  var aMessages = [];
  var counter = -1;
  var scriptToFunctions = result.scriptToFunctions;
  for (var url in scriptToFunctions) {
    counter++;
    var aFunctions = scriptToFunctions[url];

    // If the script does not contain any uncalled functions, skip it.
    if (aFunctions.uncalledCountOrSize < 1) {
      continue;
    }

    var label = 'jsCoverageRule' + counter;
    var uncalledFunctions = aFunctions.uncalledFunctions;
    var type = result.trackFunctionSizes ? 'bytes' : 'functions';
    var warning = [PAGESPEED.Utils.linkify(url), ' ',
        aFunctions.uncalledCountOrSize, ' ', type, ' uncalled of ',
        aFunctions.totalCountOrSize, ' total ', type, '. ',
        '(<a href=# onclick="document.toggleView(document, \'', label,
        '\')">Toggle Function View</a>)'].join('');

    var source = ['<div id="', label, '" style="display:none;',
        'border:1px solid #888888"/>'];
    source.push(['Uncalled functions: ',
                 '<br/><pre style="color:#000;overflow:auto">',
                 PAGESPEED.Utils.htmlEscape(uncalledFunctions.join('\n')),
                 '</pre>'].join(''));
    source.push('</div>');

    aMessages.push([warning, source.join('')].join(''));
  }

  // If it's a cached result, then tell the user, and show a
  // link to the info page for this rule.
  if (cached) {
    var linkStr =
      [' (<a href="', URL_JS_COVERAGE_WIKI,
       '" target=_blank>Cached Result</a>)'].join('');
  }

  var warningStr =
      [PAGESPEED.Utils.formatPercent(percentageUncalled), ' of the ',
       'JavaScript loaded by this page had not been invoked by ',
       'the time the onload handler completed.'].join('');

  this.warnings = [warningStr, linkStr,
                   PAGESPEED.Utils.formatWarnings(aMessages, true)].join('');
};

var cleanUpResults = function(result, context) {
  if (!result || !context) {
    return;
  }

  if (result.cleanedUp) {
    // already cleaned up
    return;
  }
  result.cleanedUp = true;

  if (!result.profilerRan) {
    result.failedToProfile = true;
  }

  var currentUrl = context.browser.currentURI.spec;
  var scriptCache = getScriptCache();
  var scriptToFunctions = result.scriptToFunctions;

  for (var url in scriptToFunctions) {
    if (scriptCache[url] == null) {
      // We're the first page to reference this JS resource, so
      // indicate that by creating an entry in the scriptCache for the
      // resource's URL.
      scriptCache[url] = currentUrl;
    } else if (scriptCache[url] != currentUrl) {
      // Some other page already profiled using this JS resource, so
      // we can't generate an accurate score for this page.
      if (!result.scriptCollisions) {
        result.scriptCollisions = {};
      }

      // Remember that there's a scriptCollision, and store away the
      // URL of the page that originally loaded the shared JS
      // resource.
      result.scriptCollisions[url] = scriptCache[url];
    }
  }

  var maxFnsPerJs = PAGESPEED.Utils.getIntPref(MAX_FUNCTIONS_PER_SCRIPT_PREF,
                                           MAX_FUNCTIONS_PER_SCRIPT_DEFAULT);
  if (scriptToFunctions) {
    // Now clean up the arrays that contain the source code of the
    // various JS functions.
    for (var url in scriptToFunctions) {
      var aFunctions = scriptToFunctions[url];

      // Clean up the source code for each function. We do this here,
      // rather than in the profiling function, because the profiling
      // function can run during page load, so we want to minimize its
      // overhead.
      for (var i = 0, len = aFunctions.uncalledFunctions.length;
           i < len;
           i++) {
        aFunctions.uncalledFunctions[i] =
          PAGESPEED.Utils.cleanUpJsdScriptSource(
              aFunctions.uncalledFunctions[i]);
      }

      // Sort the list of functions so they are presented from longest
      // to shortest.
      var sortByFunctionLength = function(a, b) {
        return b.length - a.length;
      };
      aFunctions.uncalledFunctions.sort(sortByFunctionLength);

      // Sometimes there are thousands of uncalled
      // functions. Truncate to a more reasonable number, so the UI
      // doesn't become unresponsive due to insanely long lists. We
      // perform the truncation in the profiling method, to limit
      // the size of the cached object.
      if (aFunctions.uncalledFunctions.length >= maxFnsPerJs) {
        aFunctions.uncalledFunctions.splice(maxFnsPerJs);
        aFunctions.uncalledFunctions.push('...truncated at ' + maxFnsPerJs +
                                          ' functions...');
      }
    }
  }
};

var getErrorTextOrNull = function(result) {
  if (!result) {
    return 'Unable to profile JavaScript (incomplete context).';
  }

  if (!result.profilerRan) {
    if (result.failedToProfile) {
      // The context object for a tab doesn't get constructed until
      // that tab becomes visible. When this happens, we don't end up
      // starting the profiler in time, and the profiler never gets
      // called. The fix is to reload the page, at which point we will
      // properly profile it. Unfortuantely, profiling a reloaded page
      // can yield inaccurate results, so we encourage the user to
      // restart the browser.
      return ['Unable to profile JavaScript. Try restarting the browser, ',
              'making sure to run Page Speed the first time this page is ',
              'opened. (<a href="', URL_JS_COVERAGE_WIKI, '" target=_blank>',
              'More Info</a>)'].join('');
    } else {
      // error case. should never happen.
      return 'Unable to profile JavaScript (profiler hook not invoked).';
    }
  }

  if (result.scriptCollisions) {
    var aCollidingPages = [];
    for (var url in result.scriptCollisions) {
      aCollidingPages.push(result.scriptCollisions[url]);
    }

    return ['Unable to profile JavaScript. This page shares JavaScript ',
            'resources with one or more pages that have already been visited ',
            'during this browsing session (', aCollidingPages.join(', '),
            '). Try restarting the browser, making sure to run Page Speed on ',
            'this page as soon as the browser opens. (<a href="',
            URL_JS_COVERAGE_WIKI, '" target=_blank>More Info</a>)'].join('');
  }

  return null;
};

/**
 * @constructor
 */
var CachedResult = function() {
  this.trackFunctionSizes =
      PAGESPEED.Utils.getBoolPref(TRACK_FUNCTION_SIZES_PREF,
                              TRACK_FUNCTION_SIZES_DEFAULT);
};

CachedResult.prototype.profilerRan = false;
CachedResult.prototype.scriptToFunctions = null;
CachedResult.prototype.totalCountOrSize = 0;
CachedResult.prototype.uncalledCountOrSize = 0;
CachedResult.prototype.failedToProfile = false;
CachedResult.prototype.cleanedUp = false;

/**
 * @constructor
 */
var CoverageProfiler = function(context, result) {
  this.context = context;
  this.result = result;
  // This is required to make the firebug service happy.
  this.wrappedJSObject = { supportsGlobal: function() { return false; } };
  this.startProfiling();
};

CoverageProfiler.prototype.profiling = false;
CoverageProfiler.prototype.listening = false;

/**
 * Implements nsISupports.
 */
CoverageProfiler.prototype.QueryInterface = function(iid) {
  if (iid.equals(nsIWebProgressListener) ||
      iid.equals(nsISupportsWeakReference) ||
      iid.equals(nsISupports)) {
    return this;
  }
  throw Components.results.NS_NOINTERFACE;
};

/**
 * Called when the state of a loading document changes. This includes
 * both the main page, and any embedded iframes within that page. See
 * http://developer.mozilla.org/en/docs/nsIWebProgressListener
 */
CoverageProfiler.prototype.onStateChange = function(aWebProgress,
                                                    aRequest,
                                                    aStateFlags,
                                                    aStatus) {
  if ((aStateFlags & STATE_STOP) &&
      (aStateFlags & STATE_IS_WINDOW)) {
    // Remove the hash from the URLs, if present.
    var contextUrl = this.context.browser.currentURI.spec.replace(/[#].*/, '');
    var requestUrl = aRequest.name.replace(/[#].*/, '');
    // This function gets called for the page, as well as each
    // resource in the page. We only want to trigger for the main
    // page, so make sure the URL of the request matches that of the
    // main page.
    if (requestUrl == contextUrl) {
      this.profileScripts();
      this.stopProfiling();
    }
  }
};

/**
 * Iterate over all of the js functions that the firefox debugger
 * knows about, and see which were called and which were not.
 */
CoverageProfiler.prototype.profileScripts = function() {
  var prefs = PAGESPEED.Utils.getPrefs();
  if (!prefs.getBoolPref(JS_PROFILER_ENABLE_PREF)) {
    // The user has disabled the profiler, so don't do any profiling.
    return;
  }

  if (this.result.profilerRan) {
    // Already ran, so don't run again.
    return;
  }

  // Get a list of the URLs for all resources that can include
  // javascript on this page. This includes the main page, iframes,
  // and js files.
  var scriptFiles = getScriptFiles(this.context);

  // We will build a map from script URLs to an array of uncalled
  // functions within that script. We only store a limited number (set by
  // preference extensions.PageSpeed.max_functions_per_script)
  // for each script URL, so we keep the actual count of uncalled
  // functions in the "totalCountOrSize" property.
  var scriptToFunctions = {};

  for (var component in scriptFiles) {
    scriptToFunctions[component] = {};
    scriptToFunctions[component].totalCountOrSize = 0;
    scriptToFunctions[component].uncalledCountOrSize = 0;
    scriptToFunctions[component].uncalledFunctions = [];
  }

  var enumerator = new ScriptEnumerator(
      scriptFiles, scriptToFunctions, this.result.trackFunctionSizes);

  // Hand our script enumerator object to the firefox debugger, so it
  // can call our implementation of enumerateScript for each JS
  // function it knows about.
  try {
    FBL.jsd.enumerateScripts(enumerator);
  } catch (e) {
    PS_LOG('ERROR invoking JavaScript debugger (FBL.jsd):\n' + e);
    return;
  }

  // Persist the computed data to the cached object so we can retrieve
  // it later.
  this.result.scriptToFunctions = scriptToFunctions;
  this.result.totalCountOrSize = enumerator.totalCountOrSize;
  this.result.uncalledCountOrSize = enumerator.uncalledCountOrSize;
  this.result.profilerRan = true;
};

CoverageProfiler.prototype.isProfiling = function() {
  return this.profiling;
};

CoverageProfiler.prototype.startProfiling = function() {
  if (this.profiling) {
    return;
  }

  FBL.fbs.registerDebugger(this);
  FBL.fbs.startProfiling();
  this.profiling = true;

  // Configure a listener that will fire right after onload. This
  // listener is used to gather the URIs that are loaded as part of the page.
  this.context.browser.addProgressListener(this, NOTIFY_STATE_WINDOW);
  this.listening = true;
};

CoverageProfiler.prototype.stopProfiling = function() {
  if (!this.profiling) {
    return;
  }

  this.unlisten();

  FBL.fbs.stopProfiling();
  FBL.fbs.unregisterDebugger(this);
  this.profiling = false;
};

CoverageProfiler.prototype.isListening = function() {
  return this.listening;
};

CoverageProfiler.prototype.unlisten = function() {
  if (!this.listening) {
    return;
  }

  this.context.browser.removeProgressListener(this);
  this.listening = false;
};

// We define these functions to fulfill nsIWebProgressListener.
CoverageProfiler.prototype.onProgressChange = function() {};
CoverageProfiler.prototype.onLocationChange = function() {};
CoverageProfiler.prototype.onStatusChange = function() {};
CoverageProfiler.prototype.onSecurityChange = function() {};

/**
 * @constructor
 */
var ScriptEnumerator = function(
    scriptFiles, scriptToFunctions, trackFunctionSizes) {
  this.scriptToFunctions = scriptToFunctions;
  this.scriptFiles = scriptFiles;
  this.trackFunctionSizes = trackFunctionSizes;

  // Get the maximum number of javascript functions to report.
  // Done here rather tan in ScriptEnumerator.prototype.enumerateScript
  // because there is no need to reread the pref for every js function.
  this.maxFnsPerJs =
      PAGESPEED.Utils.getIntPref(MAX_FUNCTIONS_PER_SCRIPT_PREF,
                             MAX_FUNCTIONS_PER_SCRIPT_DEFAULT);
};

ScriptEnumerator.prototype.totalCountOrSize = 0;
ScriptEnumerator.prototype.uncalledCountOrSize = 0;

/**
 * Implement jsdIScriptEnumerator. This function gets called once
 * for each function known to the JavaScript debugger.
 */
ScriptEnumerator.prototype.enumerateScript = function(script) {
  var url = script.fileName;
  // Only include this function in our analysis if the file it
  // came from is a component of the page we're evaluating. This
  // means that all scripts referenced from the document head
  // and body will be included. Scripts that are prefetched via
  // an XHR in the onload handler will not be included.
  if (this.scriptFiles[url]) {
    var scriptData = this.scriptToFunctions[url];
    this.processFunction(script, scriptData);
    // Unfortunately, there appears to a be a bug in the Firefox JS
    // debugger service that causes Firefox to crash when we call this
    // method. It's not critical that we clear the profile data, so we
    // leave this commented out for now.
    //
    // script.clearProfileData();
  }
};

ScriptEnumerator.prototype.processFunction = function(script, scriptData) {
  var countOrSize = 0;
  if (this.trackFunctionSizes) {
    countOrSize = PAGESPEED.Utils.getFunctionSize(script.functionSource);
  } else {
    countOrSize = 1;
  }
  scriptData.totalCountOrSize += countOrSize;
  this.totalCountOrSize += countOrSize;

  if (script.callCount > 0) {
    return;
  }

  scriptData.uncalledCountOrSize += countOrSize;
  this.uncalledCountOrSize += countOrSize;

  if (scriptData.uncalledFunctions.length < this.maxFnsPerJs) {
    scriptData.uncalledFunctions.push(script.functionSource);
  }
};


var getScriptFiles = function(context) {
  // Get the list of resources referenced by this page that might
  // contain javascript. This includes js files and HTML pages (main
  // page and iframes).
  var scriptPanel = context.getPanel('script');
  if (scriptPanel == null) {
    throw {message: 'Please make sure script panel is enabled in Firebug.'};
  }

  if (Firebug.Debugger && Firebug.Debugger.updateScriptFiles) {
    Firebug.Debugger.updateScriptFiles(context);
  } else if (scriptPanel.updateScriptFiles) {
    scriptPanel.updateScriptFiles(context);
  } else if(FBL.updateScriptFiles) {
    FBL.updateScriptFiles(context);
  } else {
    throw {message: 'Cannot update script files in Firebug.'};
  }

  // Sanity check the component map and remove any non-HTTP
  // URLs. Skype seems to inject a chrome URL into each page, for
  // instance, and we don't want to include those URLs in our
  // analysis.
  var scriptFiles = {};
  for (var scriptUri in context.sourceFileMap) {
    if (scriptUri.match(/^http/)) {
      scriptFiles[scriptUri] = true;
    }
  }

  return scriptFiles;
};

var isTopLevelFunction = function(script) {
  // A top-level function has a null parent function.
  return script.functionObject.jsParent.jsFunctionName == null;
};

var getScoreForPercentage = function(percentage) {
  if (percentage >= .7) {
    // 70-100 percent coverage is a score of 100 (A).
    return 100;
  }

  if (percentage <= .3) {
    // 0-30 percent maps to a score of 0-60 (an F).
    return Math.round(percentage * 200);
  }

  // 30-70 percent gets mapped to a score range of 60-100 (D to A).
  return Math.round((100 * percentage) + 30);
};

var getCacheRoot = function() {
  return PAGESPEED.Utils.getCachedObject('jsCoverageRule');
};

var getScriptCache = function() {
  var cachedResults = getCacheRoot();
  if (!cachedResults[SCRIPT_CACHE_NAME]) {
    cachedResults[SCRIPT_CACHE_NAME] = {};
  }

  return cachedResults[SCRIPT_CACHE_NAME];
};

/**
 * If this is the first time that this page has been visited, create
 * a new object to cache the profiling results in. Otherwise, return
 * the existing result object, which may or may not be populated.
 */
var fetchOrCreateCachedResult = function(context) {
  var uri = PAGESPEED.Utils.getBaseUrl(context.browser.currentURI.spec);

  var cachedResults = getCacheRoot();
  if (!cachedResults[uri]) {
    var result = new CachedResult();
    cachedResults[uri] = result;
  }

  return cachedResults[uri];
};

var doesCachedResultExist = function(context) {
  var cachedResults = getCacheRoot();
  var uri = PAGESPEED.Utils.getBaseUrl(context.browser.currentURI.spec);
  return cachedResults[uri] != null;
};

var clearCachedResult = function(context) {
  var cachedResults = getCacheRoot();
  var uri = PAGESPEED.Utils.getBaseUrl(context.browser.currentURI.spec);
  cachedResults[uri] = null;
};

var isFirebugApiAvailable = function() {
  var updateScriptFilesAvailable = false;
  // In FB 1.7, updateScriptFiles is a method on the Debugger.
  if (Firebug &&
      Firebug.Debugger &&
      Firebug.Debugger.updateScriptFiles) {
    updateScriptFilesAvailable = true;
  }
  // In FB 1.5, updateScriptFiles is a method on the ScriptPanel.
  if (!updateScriptFilesAvailable &&
      Firebug &&
      Firebug.ScriptPanel &&
      Firebug.ScriptPanel.prototype &&
      Firebug.ScriptPanel.prototype.updateScriptFiles) {
    updateScriptFilesAvailable = true;
  }
  // Prior to FB 1.5, updateScriptFiles was a method on FBL.
  if (!updateScriptFilesAvailable &&
      FBL &&
      FBL.updateScriptFiles) {
    updateScriptFilesAvailable = true;
  }
  if (!updateScriptFilesAvailable) {
    return false;
  }

  return FBL &&
      FBL.jsd &&
      FBL.fbs &&
      FBL.fbs.registerDebugger &&
      FBL.fbs.unregisterDebugger &&
      FBL.fbs.startProfiling &&
      FBL.fbs.stopProfiling;
};

//
// The LintRule instance for this rule, with a few added functions.
//

var rule = new PAGESPEED.LintRule(
  'Defer loading of JavaScript',
  PAGESPEED.PAYLOAD_GROUP,
  'payload.html#DeferLoadingJS',
  jsCoverageRule,
  2.25,
  'DeferJS'
);

/**
 * Called whenever the browser navigates to a new page.
 */
rule.initContext = function(context) {
  context.jsCoverageContext = {};
  var cachedResultExists = doesCachedResultExist(context);
  var result = fetchOrCreateCachedResult(context);
  context.jsCoverageContext.result = result;

  if (!cachedResultExists) {
    // We created the cached result, so we need to create a
    // CoverageProfiler to profile the page and fill out the result
    // object.
    var prefs = PAGESPEED.Utils.getPrefs();
    if (prefs.getBoolPref(JS_PROFILER_ENABLE_PREF) &&
        isFirebugApiAvailable()) {
      context.jsCoverageContext.profiler =
        new CoverageProfiler(context, result);
    } else {
      // Keep track of the fact that we failed to profile this page,
      // so we can report the correct error message to the
      // user.
      result.failedToProfile = true;
    }
  }
};

/**
 * Called whenever the browser navigates away from a page.
 */
rule.destroyContext = function(context) {
  if (context && context.jsCoverageContext) {
    var jsCoverageContext = context.jsCoverageContext;

    if (jsCoverageContext.profiler) {
      if (jsCoverageContext.profiler.isListening()) {
        jsCoverageContext.profiler.unlisten();
      }

      if (jsCoverageContext.profiler.isProfiling()) {
        jsCoverageContext.profiler.stopProfiling();
      }

      cleanUpResults(jsCoverageContext.result, context);

      jsCoverageContext.profiler = null;
    }

    jsCoverageContext.result = null;

    context.jsCoverageContext = null;
  }
};

PAGESPEED.LintRules.registerLintRule(rule);

})();  // End closure
