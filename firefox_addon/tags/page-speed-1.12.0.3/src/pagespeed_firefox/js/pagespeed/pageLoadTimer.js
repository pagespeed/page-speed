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
 * @fileoverview Track the time a page takes to load.
 *
 * Use an eventListner to get the time a page starts to load and the time a
 * page is finished loading.  Subtract to get the time to load the page.
 *
 * Docs on the web progress listner interface used to compute page load
 * times can be found at:
 * https://developer.mozilla.org/en/nsIWebProgressListener
 *
 * @author Sam Kerner
 */

(function() {  // Begin closure

// If a page is loaded at startup, it is posible that we will not install the
// page load timer before the page fires the start event.  In this case, use
// the time the browser loaded this file.
var timePageSpeedLoaded = (new Date()).getTime();

var nsIWebProgressListener = Components.interfaces.nsIWebProgressListener;

// Start timing on a start event that applies to a document.
var START_FILTER = (nsIWebProgressListener.STATE_START |
                    nsIWebProgressListener.STATE_IS_DOCUMENT);

// Stop timing on a stop event that ends a network event from a window.
var STOP_FILTER = (nsIWebProgressListener.STATE_STOP |
                   nsIWebProgressListener.STATE_IS_NETWORK |
                   nsIWebProgressListener.STATE_IS_WINDOW);

/**
 * pageLoadTimer is passed to window.addProgressListener().  It implements the
 * nsIWebProgressListener interface.  See
 * http://developer.mozilla.org/en/docs/nsIWebProgressListener .
 * @param {boolean} couldHaveMissedStart If true, the start of the page load
 *     may have already happened.
 * @constructor
 */
PAGESPEED.PageLoadTimer = function(couldHaveMissedStart) {
  /**
   * The window whose timing we are measuring.  null when no measurement
   * is in progress.
   * @type {nsIDOMWindow}
   * @private
   */
  this.window_ = null;
  this.couldHaveMissedStart_ = couldHaveMissedStart;
};

PAGESPEED.PageLoadTimer.prototype.QueryInterface = function(aIID) {
  if (aIID.equals(Components.interfaces.nsIWebProgressListener) ||
      aIID.equals(Components.interfaces.nsISupportsWeakReference) ||
      aIID.equals(Components.interfaces.nsISupports))
    return this;

  throw Components.results.NS_NOINTERFACE;
};

/**
 * Function onStateChange is called when a change in the state of the page
 * being loaded occurs.  We are interested in the start state (when the page
 * starts to load), and the stop state (when the page finishes loading).
 *
 * @param {nsIWebProgress} aProgress The nsIWebProgress that fired the
 *     notification.
 * @param {nsIRequest} aRequest The nsIRequest that has changed state.
 * @param {Number} aStatus Flags indicating the new state.
 * @param {Number} aFlag Error status code associated with the state change.
 */
PAGESPEED.PageLoadTimer.prototype.onStateChange = function(
    aProgress, aRequest, aFlag, aStatus) {
  // Do not try to time requests to non-http or https URLs.
  // Note that a dummy request to "about:document-onload-blocker" is
  // fired by the DOM inspector extension.  This test filters it.
  // Docs say aRequest may be null, and has a note saying the meaning
  // aRequest===null needs to be documented.
  if (aRequest && (!aRequest.name || !/^https?:/.test(aRequest.name)))
    return;

  // Only time the top level window.  Ignore sub-windows, such as frames.
  if (aProgress.DOMWindow !== aProgress.DOMWindow.top)
    return;

  if ((aFlag & START_FILTER) == START_FILTER) {
    // Only record the window and start the timer if there is not already
    // a timer running.  Stop the timer when we see a stop event on the
    // window that we record here on a start event.  This works because
    // the top level window will have the first start event, and the last
    // stop event.
    this.window_ = aProgress.DOMWindow;
    this.startTime_ = (new Date()).getTime();
    delete this.loadTime_;
  }

  if ((aFlag & STOP_FILTER) == STOP_FILTER) {
    var startTime;

    if (this.window_ == aProgress.DOMWindow) {
      // We have a stop event for the extected window.
      startTime = this.startTime_;

    } else if (!this.window_ && this.couldHaveMissedStart_) {
      // PageLoadTimer objects are installed on tabs as they are opened.
      // Tabs that the browser opens at startup may start loading before
      // the code in this file can add an evenrt listner, so we may miss
      // a start event.  In this case, this.couldHaveMissedStart_ was set.
      // Since this can only happen when the page started loading as the
      // browser started up, use the time page speed was loaded as the
      // start time.
      startTime = timePageSpeedLoaded;
    }

    // If we saw a start time, compute the page load time.
    if (startTime) {
      this.window_ = null;
      this.loadTime_ = (new Date()).getTime() - startTime;

      // We got a valid stop event, so there is no way we will not be listening
      // for the next start event.
      this.couldHaveMissedStart_ = false;
    }
  }

  return;
};

// The next five functions are required by the nsIWebProgressListener
// interface, but do not do anything for Page Speed.
PAGESPEED.PageLoadTimer.prototype.onLocationChange = function(
    aProgress, aRequest, aURI) {return;};
PAGESPEED.PageLoadTimer.prototype.onProgressChange = function() {return;};
PAGESPEED.PageLoadTimer.prototype.onStatusChange = function() {return;};
PAGESPEED.PageLoadTimer.prototype.onSecurityChange = function() {return;};
PAGESPEED.PageLoadTimer.prototype.onLinkIconAvailable = function() {return;};


/**
 * Given a tabBrowser XUL object, get the time the last page load took.
 * @param {Object} browserTab The browser object which represents the tab
 *    scores were computed in.
 * @return {number|undefined} The load time of the tab's page.
 */
PAGESPEED.PageLoadTimer.getPageLoadTimeByTab = function(browserTab) {
  if (!browserTab.pagespeed_ || !browserTab.pagespeed_.pageLoadTimer) {
    return undefined;
  }
  return browserTab.pagespeed_.pageLoadTimer.loadTime_;
};

/**
 * Called when a tab is created.  Adds a page load listner.
 * @param {Object} event Record used to find the browser object of the new tab.
 * @param {boolean} opt_couldHaveMissedStart If true, this function
 *     is being called after the event that created the tab, meaning the
 *     page load listener might be installed after the page in the tab
 *     starts loading.
 */
function installPageLoadTimerOnTab(event, opt_couldHaveMissedStart) {
  // browser is the XUL element of the browser that's been added.
  var browserTab = event.target.linkedBrowser;

  if (browserTab.pagespeed_ && browserTab.pagespeed_.pageLoadTimer) {
    // Already installed.  This should not happen.
    PS_LOG('Bug in page load timer: Install function called on the same ' +
           'tab twice.');
    return;
  }

  var plt = new PAGESPEED.PageLoadTimer(!!opt_couldHaveMissedStart);

  var NSD = Components.interfaces.nsIWebProgress.NOTIFY_STATE_DOCUMENT;
  browserTab.addProgressListener(plt, NSD);

  // TODO: If other parts of PageSpeed need tab-specific state,
  // then break the tab specific code into its own file and
  // store other state under to pagespeed object.
  browserTab.pagespeed_ = {
    pageLoadTimer: plt
  };
}

/**
 * Called when a tab is destroyed.  Remove its page load listner.
 * @param {Object} event Record used to find the browser object of the new tab.
 */
function removePageLoadTimerOnTab(event) {
  var browserTab = event.target.linkedBrowser;

  var plt = browserTab.pagespeed_.pageLoadTimer;
  browserTab.removeProgressListener(plt);

  delete browserTab.pagespeed_.pageLoadTimer;
  delete browserTab.pagespeed_;
}

/**
 * Called on the load event of the first tab in a window, this function
 * installs event listeners for tab creation and destruction.  Because
 * the initial tab and tabs saved from a previous session may already
 * exist, this function also calls the TabOpen event callback on any
 * tabs that already exist.
 */
function installTabListeners() {
  var gBrowser = PAGESPEED.Utils.getGBrowser();
  var container = gBrowser.tabContainer;
  container.addEventListener('TabOpen', installPageLoadTimerOnTab, false);
  container.addEventListener('TabClose', removePageLoadTimerOnTab, false);

  // For any tab that already exists, call installPageLoadTimerOnTab().
  var numTabs = gBrowser.browsers.length;
  for (var i = 0; i < numTabs; ++i) {
    var browserTab = gBrowser.getBrowserAtIndex(i);
    installPageLoadTimerOnTab({target: {linkedBrowser: browserTab}}, true);
  }
}

/**
 * Called on the unload event of a window, this function
 * removes event listeners for tab creation and destruction.
 * Closing a window does not cause tabClose events for each tab
 * in that window, so tabClose events are faked for all open tabs.
 */
function removeTabListeners() {
  var gBrowser = PAGESPEED.Utils.getGBrowser();
  var container = gBrowser.tabContainer;
  container.removeEventListener('TabOpen', installPageLoadTimerOnTab, false);
  container.removeEventListener('TabClose', removePageLoadTimerOnTab, false);

  // Closing a firefox window will not cause TabClose events to fire for each
  // tab in the window.  Manualy clean up each tab:
  var numTabs = gBrowser.browsers.length;
  for (var i = 0; i < numTabs; ++i) {
    var browserTab = gBrowser.getBrowserAtIndex(i);
    removePageLoadTimerOnTab({target: {linkedBrowser: browserTab}});
  }
}

// For each firefox window (not each tab), this file will be sourced, and
// a load event will cause installTabListeners() to be called.
window.addEventListener('load', installTabListeners, false);
window.addEventListener('unload', removeTabListeners, false);

})();  // End closure
