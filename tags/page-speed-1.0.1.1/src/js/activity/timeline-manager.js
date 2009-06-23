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
 * @fileoverview TimelineManager manages the timeline event sources,
 * as well as the timeline model and view instances.
 *
 * @author Bryan McQuade
 */

goog.provide('activity.TimelineManager');

goog.require('goog.dispose');
goog.require('activity.JsEventFetcher');
goog.require('activity.NetworkObserver');
goog.require('activity.RequestObserver');
goog.require('activity.TimelineModel');
goog.require('activity.TimelineView');

/**
 * The TimelineManager constructs and manages the timeline event
 * sources, as well as the timeline model and view instances.
 * @param {Object} timeoutFactory object that provides setTimeout,
 *     clearTimeout (e.g. the global window object).
 * @param {Function} callbackWrapper wrapper function that handles
 *     exceptions thrown by a callback function.
 * @constructor
 */
activity.TimelineManager = function(timeoutFactory, callbackWrapper) {
  /**
   * Object that provides setTimeout, clearTimeout, etc.
   * @type {Object}
   * @private
   */
  this.timeoutFactory_ = timeoutFactory;

  /**
   * Function that handles exceptions thrown by a callback function.
   * @type {Function}
   * @private
   */
  this.callbackWrapper_ = callbackWrapper;

  /**
   * @type {Function}
   * @private
   */
  this.boundOnTimeoutCallback_ = goog.partial(
    callbackWrapper, goog.bind(this.onTimeoutCallback_, this));
};

/**
 * Possible states for the timeline manager.
 * @enum {number}
 */
activity.TimelineManager.State = {
  NOT_STARTED: 0,
  STARTED: 1,
  FINISHED: 2
};

/**
 * The preference name for the resolution of the UI.
 * @type {string}
 * @private
 */
activity.TimelineManager.PREF_RESOLUTION_MSEC_ = 'ui_resolution_msec';

/**
 * The default resolution of the timeline view, in milliseconds.
 * @type {number}
 * @private
 */
activity.TimelineManager.DEFAULT_RESOLUTION_MSEC_ = 20;

/**
 * The preference name for the UI refresh delay.
 * @type {string}
 * @private
 */
activity.TimelineManager.PREF_REFRESH_DELAY_MSEC_ = 'ui_refresh_delay_msec';

/**
 * The default UI refresh delay, in milliseconds.
 * @type {number}
 * @private
 */
activity.TimelineManager.DEFAULT_REFRESH_DELAY_MSEC_ = 150;

/**
 * The js event fetcher, which queries the IActivityProfiler instance for
 * js timeline events, and pushes those events into the TimelineModel.
 * @type {activity.JsEventFetcher?}
 * @private
 */
activity.TimelineManager.prototype.fetcher_ = null;

/**
 * @type {activity.NetworkObserver?}
 * @private
 */
activity.TimelineManager.prototype.network_observer_ = null;

/**
 * @type {activity.RequestObserver?}
 * @private
 */
activity.TimelineManager.prototype.request_observer_ = null;

/**
 * The timeline model, which dispatches timeline events to the view.
 * @type {activity.TimelineModel?}
 * @private
 */
activity.TimelineManager.prototype.model_ = null;

/**
 * The timeline view, which renders the execution timeline.
 * @type {activity.TimelineView?}
 * @private
 */
activity.TimelineManager.prototype.view_ = null;

/**
 * The handle returned from setTimeout, which we can use to cancel
 * a timer callback.
 * @type {Object}
 * @private
 */
activity.TimelineManager.prototype.timeoutId_ = null;

/**
 * Amount of time between calls to onTimeoutCallback_, in microseconds.
 * @type {number}
 * @private
 */
activity.TimelineManager.prototype.timerDelayMsec_ = -1;

/**
 * State of the TimelineManager
 * @type {activity.TimelineManager.State}
 * @private
 */
activity.TimelineManager.prototype.state_ =
    activity.TimelineManager.State.NOT_STARTED;

/**
 * Start profiling, and populate the JavaScript execution timeline UI.
 * @param {IActivityProfiler} activityProfiler the IActivityProfiler instance.
 * @param {nsIObserverService} observerService the observer service.
 * @param {number} startTimeUsec the start time, in microseconds.
 * @param {nsIDOMDocument} xulElementFactory A factory for creating DOM
 *     elements (e.g. the document object).
 * @param {nsIDOMElement} xulRowsElement The XUL rows element that we
 *     will contain the UI timeline.
 */
activity.TimelineManager.prototype.start = function(
    activityProfiler,
    observerService,
    startTimeUsec,
    xulElementFactory,
    xulRowsElement) {
  this.reset();

  this.timerDelayMsec_ = activity.preference.getInt(
      activity.TimelineManager.PREF_REFRESH_DELAY_MSEC_,
      activity.TimelineManager.DEFAULT_REFRESH_DELAY_MSEC_);

  var resolutionMsec = activity.preference.getInt(
      activity.TimelineManager.PREF_RESOLUTION_MSEC_,
      activity.TimelineManager.DEFAULT_RESOLUTION_MSEC_);

  if (resolutionMsec < 2) {
    // limit to 2ms resolution minimum
    resolutionMsec = 2;
  }

  this.view_ = new activity.TimelineView(
      xulRowsElement,
      xulElementFactory,
      resolutionMsec * 1000);

  this.model_ = new activity.TimelineModel();
  this.model_.addListener(this.view_);

  this.fetcher_ = new activity.JsEventFetcher(
    this.model_,
    this.view_.getResolutionUsec(),
    activityProfiler,
    this.callbackWrapper_);

  this.network_observer_ = new activity.NetworkObserver(
    activityProfiler,
    observerService,
    this.model_,
    startTimeUsec,
    this.view_.getResolutionUsec());

  this.request_observer_ = new activity.RequestObserver(
    this.timeoutFactory_,
    activityProfiler,
    observerService,
    this.model_,
    startTimeUsec,
    this.view_.getResolutionUsec());

  this.fetcher_.start();
  this.network_observer_.register();
  this.request_observer_.register();
  this.timeoutId_ = this.timeoutFactory_.setTimeout(
      this.boundOnTimeoutCallback_, this.timerDelayMsec_);

  this.state_ = activity.TimelineManager.State.STARTED;
};

/**
 * Stop profiling.
 */
activity.TimelineManager.prototype.stop = function() {
  if (this.fetcher_) {
    this.fetcher_.stop();
  }

  if (this.network_observer_) {
    this.network_observer_.unregister();
  }
  if (this.request_observer_) {
    this.request_observer_.unregister();
  }
  this.state_ = activity.TimelineManager.State.FINISHED;

  // We don't want to release the other timeline objects yet, because the
  // fetcher might still have some asynchronous tasks to perform (e.g.
  // catching the timeline up to "now"). So, we tell the fetcher
  // to stop querying for new events once it's caught up.
};

/**
 * Stop profiling and tear down the UI.
 */
activity.TimelineManager.prototype.reset = function() {
  this.stop();

  if (this.view_) {
    goog.dispose(this.view_);
  }

  if (this.model_) {
    this.model_.removeListener(this.view_);
    goog.dispose(this.model_);
  }

  if (this.fetcher_) {
    goog.dispose(this.fetcher_);
  }

  if (this.network_observer_) {
    goog.dispose(this.network_observer_);
  }

  if (this.request_observer_) {
    goog.dispose(this.request_observer_);
  }

  if (this.timeoutId_) {
    this.timeoutFactory_.clearTimeout(this.timeoutId_);
  }

  this.view_ = null;
  this.model_ = null;
  this.fetcher_ = null;
  this.network_observer_ = null;
  this.request_observer_ = null;
  this.timeoutId_ = null;
  this.state_ = activity.TimelineManager.State.NOT_STARTED;
};

/**
 * @return {activity.TimelineManager.State} The current state of this
 *     TimelineManager.
 */
activity.TimelineManager.prototype.getState = function() {
  return this.state_;
};

/**
 * Called when our timer fires. We in turn tell each of the event
 * fetchers that the timer has fired, and optionally schedule the
 * timer to fire again.
 */
activity.TimelineManager.prototype.onTimeoutCallback_ = function() {
  var fetcherResult = this.fetcher_.onTimeoutCallback();
  var networkResult = this.network_observer_.onTimeoutCallback();
  var requestResult = this.request_observer_.onTimeoutCallback();

  var shouldRunAgain = fetcherResult || networkResult || requestResult;
  if (shouldRunAgain) {
    this.timeoutId_ = this.timeoutFactory_.setTimeout(
        this.boundOnTimeoutCallback_, this.timerDelayMsec_);
  } else {
    this.timeoutId_ = null;
  }
};
