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
 * @fileoverview JsEventFetcher fetches JavaScript timeline events
 * and passes those events to the TimelineModel.
 * @author Bryan McQuade
 */

goog.provide('activity.JsEventFetcher');

goog.require('goog.Disposable');
goog.require('activity.TimelineEventType');
goog.require('activity.TimelineModel');
goog.require('activity.TimelineModel.Event');

/**
 * JsEventFetcher fetches timeline events from the IActivityProfiler instance.
 *
 * @param {!activity.TimelineModel} timelineModel the timeline model.
 * @param {number} resolutionUsec the resolution, in microseconds.
 * @param {!IActivityProfiler} activityProfiler the IActivityProfiler instance.
 * @param {Function} callbackWrapper wrapper function that handles
 *     exceptions thrown by a callback function.
 * @constructor
 * @extends {goog.Disposable}
 */
activity.JsEventFetcher = function(timelineModel,
                                 resolutionUsec,
                                 activityProfiler,
                                 callbackWrapper) {
  goog.Disposable.call(this);

  this.timelineModel_ = timelineModel;
  this.resolutionUsec_ = resolutionUsec;
  this.activityProfiler_ = activityProfiler;

  /**
   * @type {Function}
   */
  var boundUpdateTimelineModel = goog.partial(
      callbackWrapper, goog.bind(this.updateTimelineModel_, this));

  /**
   * The callback we pass into the IActivityProfiler instance.
   * @type {IActivityProfilerTimelineEventCallback}
   */
  this.processTimelineEventsCallback_ =
      { processTimelineEvents: boundUpdateTimelineModel };
};
goog.inherits(activity.JsEventFetcher, goog.Disposable);

/**
 * The xpconnect handle to the IActivityProfilerEvent interface.
 * @type {IActivityProfilerEvent}
 * @private
 */
activity.JsEventFetcher.IActivityProfilerEvent_ =
    activity.xpcom.CI('IActivityProfilerEvent');

/**
 * The time to start fetching timeline events, in microseconds.
 * @type {number}
 * @private
 */
activity.JsEventFetcher.prototype.startTimeUsec_ = 0;

/**
 * The target function initialization rate, in number of microseconds
 * per initialized function. If we see a rate equal to or greater than
 * this rate for a given event, we consider the event to have an "init
 * functions score" of 1.0. The init functions score is an indicator of
 * how active function initialization was within a given event. Note that
 * the value 200 was chosen empirically.
 * @type {number}
 * @private
 */
activity.JsEventFetcher.INIT_FUNCTIONS_RATE_USEC_ = 200;

/**
 * Whether or not the fetcher is running.
 * @type {boolean}
 * @private
 */
activity.JsEventFetcher.prototype.isRunning_ = false;

/**
 * Whether or not the fetcher is finished.
 * @type {boolean}
 * @private
 */
activity.JsEventFetcher.prototype.isFinished_ = false;

/**
 * Whether or not the fetcher has an activity profiler callback pending.
 * @type {boolean}
 * @private
 */
activity.JsEventFetcher.prototype.isCallbackPending_ = false;

/**
 * Start fetching timeline events.
 */
activity.JsEventFetcher.prototype.start = function() {
  this.throwIfDisposed_();

  this.startTimeUsec_ = 0;
  this.isRunning_ = true;
  this.isFinished_ = false;
  this.isCallbackPending_ = false;
};

/**
 * Stop fetching timeline events.
 */
activity.JsEventFetcher.prototype.stop = function() {
  this.throwIfDisposed_();

  this.isRunning_ = false;
};

/**
 * Release our resources.
 */
activity.JsEventFetcher.prototype.disposeInternal = function() {
  activity.JsEventFetcher.superClass_.disposeInternal.call(this);

  if (this.isRunning_) {
    this.stop();
  }
  this.timelineModel_ = null;
  this.activityProfiler_ = null;
  this.processTimelineEventsCallback_ = null;
  this.isRunning_ = false;
  this.isFinished_ = true;
  this.isCallbackPending_ = false;
};

/**
 * Throws an Error if this JsEventFetcher is disposed.
 * @private
 */
activity.JsEventFetcher.prototype.throwIfDisposed_ = function() {
  if (this.isDisposed()) {
    throw new Error('JsEventFetcher already disposed.');
  }
};

/**
 * Fetch the timeline events.
 * @return {boolean} true if onTimeoutCallback needs to be called
 *     again next time the timer fires, false otherwise.
 */
activity.JsEventFetcher.prototype.onTimeoutCallback = function() {
  if (this.activityProfiler_.getState() != this.activityProfiler_.PROFILING &&
      this.activityProfiler_.getState() != this.activityProfiler_.FINISHED) {
    return false;
  }
  if (this.isCallbackPending_) {
    return true;
  }

  this.isCallbackPending_ = true;
  this.activityProfiler_.getTimelineEvents(this.startTimeUsec_,
                                  -1,
                                  this.resolutionUsec_,
                                  this.processTimelineEventsCallback_);

  return !this.isFinished_;
};

/**
 * Update the timeline model with the given array of events.
 *
 * @param {!nsIArray} events array of IActivityProfilerEvents.
 * @private
 */
activity.JsEventFetcher.prototype.updateTimelineModel_ = function(events) {
  if (this.isDisposed()) {
    return;
  }

  var dispatcher =
      new activity.JsEventFetcher.EventDispatcher_(this.timelineModel_);
  dispatcher.populate(events);
  if (dispatcher.hasEvents()) {
    dispatcher.dispatchEvents();
    this.startTimeUsec_ = dispatcher.getMaxEventEndTimeUsec();
  }

  /*
   * We're finished once we're no longer running and there are no more
   * events to consume.
   */
  this.isFinished_ = (!this.isRunning_ && !dispatcher.hasEvents());
  this.isCallbackPending_ = false;
};

/**
 * An EventDispatcher_ is responsible for constructing an array of
 * activity.TimelineModel.Events and dispatching those events to the
 * model.
 * @param {activity.TimelineModel} model the timeline model.
 * @constructor
 * @private
 */
activity.JsEventFetcher.EventDispatcher_ = function(model) {
  /**
   * @type {activity.TimelineModel}
   * @private
   */
  this.model_ = model;

  /**
   * @type {Object}
   * @private
   */
  this.eventMap_ = {};

  /**
   * @type {Array.<activity.TimelineModel.Event>}
   * @private
   */
  this.eventArray_ = [];
};

/**
 * Populate the array of activity.TimelineModel.Events.
 * @param {!nsIArray} events array if IActivityProfilerEvents.
 */
activity.JsEventFetcher.EventDispatcher_.prototype.populate = function(events) {
  if (this.hasEvents()) {
    throw new Error('EventDispatcher_ already populated.');
  }

  /** @type {!nsISimpleEnumerator} */
  var enumerator = events.enumerate();
  while (enumerator.hasMoreElements()) {
    /** @type {!nsISupports} */
    var eventSupports = enumerator.getNext();
    /** @type {!IActivityProfilerEvent} */
    var event = activity.xpcom.QI(
        eventSupports, activity.xpcom.CI('IActivityProfilerEvent'));
    var timelineEvent = null;
    var intensity = event.getIntensity();
    switch (event.getType()) {
      case activity.JsEventFetcher.IActivityProfilerEvent_.JS_PARSE:
        timelineEvent = this.getOrCreateEvent_(
            event, activity.TimelineEventType.JS_PARSE);
        // Convert from a raw count to a rate.
        intensity = intensity * activity.JsEventFetcher.INIT_FUNCTIONS_RATE_USEC_;
        break;
      case activity.JsEventFetcher.IActivityProfilerEvent_.JS_EXECUTE:
        timelineEvent = this.getOrCreateEvent_(
            event, activity.TimelineEventType.JS_EXECUTE);
        break;
      default:
        throw new Error('Unknown event type ' + event.getType());
    }
    timelineEvent.adjustIntensity(
        Math.min(1.0, (intensity / event.getDurationUsec())));
  }
};

/**
 * Dispatch the activity.TimelineModel.Events constructed during the
 * call to populate.
 */
activity.JsEventFetcher.EventDispatcher_.prototype.dispatchEvents = function() {
  if (!this.hasEvents()) {
    return;
  }

  this.model_.startAddingEvents();
  for (var i = 0, len = this.eventArray_.length; i < len; i++) {
    this.model_.addEvent(this.eventArray_[i]);
  }
  this.model_.doneAddingEvents(this.getMaxEventEndTimeUsec());
};
/**
 * @return {boolean} whether or not this dispatcher has any events.
 */
activity.JsEventFetcher.EventDispatcher_.prototype.hasEvents = function() {
  return this.eventArray_.length > 0;
};

/**
 * @return {number} the greatest end time of all events owned by this
 *     dispatcher.
 */
activity.JsEventFetcher.EventDispatcher_.prototype.getMaxEventEndTimeUsec =
    function() {
  var max = 0;
  for (var i = 0, len = this.eventArray_.length; i < len; i++) {
    var event = this.eventArray_[i];
    var end = event.getStartTimeUsec() + event.getDurationUsec();
    if (max < end) {
      max = end;
    }
  }
  return max;
};

/**
 * Get an event if one with the same identifier, start time, and event
 * type exists already, or create a new one if no such event exists
 * already.
 * @param {IActivityProfilerEvent} event the activity profiler event.
 * @param {activity.TimelineEvent} eventType the event type.
 * @return {activity.TimelineModel.Event} the TimelineModel event.
 * @private
 */
activity.JsEventFetcher.EventDispatcher_.prototype.getOrCreateEvent_ = function(
    event, eventType) {
  var identifier = activity.JsEventFetcher.getEventIdentifier_(event);

  // An event is uniquely defined by its type,startTime,identifier
  // tuple. We build a string that uniquely represents this key and
  // use it to look up events in our map.
  var key = [eventType, event.getStartTimeUsec(), identifier].join(':');
  if (!this.eventMap_[key]) {
    // An event with that key doesn't exist already, so construct a
    // new one. Also add it to the array.
    this.eventMap_[key] = new activity.TimelineModel.Event(
        identifier,
        event.getStartTimeUsec(),
        event.getDurationUsec(),
        eventType,
        0.0);
    this.eventArray_.push(this.eventMap_[key]);
  }
  return this.eventMap_[key];
};

/**
 * @param {IActivityProfilerEvent} event the event.
 * @return {string} the identifier for the event.
 * @private
 */
activity.JsEventFetcher.getEventIdentifier_ = function(event) {
  // We special case a few types of URLs:
  // 1. All browser URLs are lumped into a single category.
  // 2. All javascript: URLs are lumped into a single category.
  var identifier = event.getIdentifier();
  if (activity.JsEventFetcher.isBrowserUrl_(identifier)) {
    identifier = 'Firefox Javascript';
  }
  if (identifier.lastIndexOf('javascript:', 0) == 0) {
    identifier = 'javascript: URL';
  }
  return identifier;
};

/**
 * @param {string} haystack the string to search in.
 * @param {string} needle the string to search for.
 * @return {boolean} whether or not the haystack starts with needle.
 * @private
 */
activity.JsEventFetcher.startsWith_ = function(haystack, needle) {
  return haystack.lastIndexOf(needle, 0) == 0;
};

/**
 * @param {string} haystack the string to search in.
 * @param {string} needle the string to search for.
 * @return {boolean} whether or not the haystack ends with needle.
 * @private
 */
activity.JsEventFetcher.endsWith_ = function(haystack, needle) {
  var expectedIndex = haystack.length - needle.length;
  return haystack.lastIndexOf(needle, expectedIndex) == expectedIndex;
};

/**
 * @param {string} url the URL to evaluate.
 * @return {boolean} whether or not the given URL is a URL for one of
 *     Firefox's JavaScript files.
 * @private
 */
activity.JsEventFetcher.isBrowserUrl_ = function(url) {
  if (activity.JsEventFetcher.startsWith_(url, 'chrome:')) {
    return true;
  }
  if (activity.JsEventFetcher.startsWith_(url, 'file:')) {
    if (activity.JsEventFetcher.endsWith_(url, '.jsm') &&
        url.indexOf('/modules/' != -1)) {
      return true;
    }
    if (activity.JsEventFetcher.endsWith_(url, '.js') &&
        url.indexOf('/components/') != -1) {
      return true;
    }
  }
  if (activity.JsEventFetcher.endsWith_(url, '.cpp')) {
    return true;
  }
  if (url == 'XStringBundle') {
    return true;
  }

  return false;
};
