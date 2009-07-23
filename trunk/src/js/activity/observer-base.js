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
 * @fileoverview Base class for event generators that use the
 * nsIObserverService.
 *
 * @author Bryan McQuade
 */

goog.provide('activity.ObserverBase');

goog.require('activity.TimelineModel');

/**
 * Construct a new ObserverBase.
 * @param {Object} currentTimeFactory object that provides getCurrentTimeUsec.
 * @param {nsIObserverService} observerService the observer service.
 * @param {string} observerTopic the topic to observe.
 * @param {!activity.TimelineModel} timelineModel the timeline model.
 * @param {number} startTimeUsec the start time of the profiling
 *     session, in microseconds.
 * @param {number} resolutionUsec the resolution, in microseconds.
 * @constructor
 * @extends {goog.Disposable}
 */
activity.ObserverBase = function(currentTimeFactory,
                                 observerService,
                                 observerTopic,
                                 timelineModel,
                                 startTimeUsec,
                                 resolutionUsec) {
  goog.Disposable.call(this);

  /**
   * @type {Object}
   * @private
   */
  this.currentTimeFactory_ = currentTimeFactory;

  /**
   * @type {nsIObserverService}
   * @private
   */
  this.observerService_ = observerService;

  /**
   * The topic to observe.
   * @type {string}
   * @private
   */
  this.observerTopic_ = observerTopic;

  /**
   * The timeline model, which dispatches timeline events to the view.
   * @type {activity.TimelineModel}
   * @protected
   */
  this.timelineModel_ = timelineModel;

  /**
   * The start time.
   * @type {number}
   * @private
   */
  this.startTimeUsec_ = startTimeUsec;

  /**
   * The resolution.
   * @type {number}
   * @private
   */
  this.resolutionUsec_ = resolutionUsec;

  /**
   * Array of events.
   * @type {Array?}
   * @private
   */
  this.events_ = null;
};
goog.inherits(activity.ObserverBase, goog.Disposable);

/**
 * Whether or not we're registered with the nsIObserverService.
 * @type {boolean}
 * @private
 */
activity.ObserverBase.prototype.isRegistered_ = false;

/**
 * The end time for this observer.
 * @type {number}
 * @private
 */
activity.ObserverBase.prototype.endTimeUsec_ = 0;

/**
 * Release our resources.
 */
activity.ObserverBase.prototype.disposeInternal = function() {
  activity.ObserverBase.superClass_.disposeInternal.call(this);

  this.unregister();
  this.timeoutFactory_ = null;
  this.timelineModel_ = null;
  this.events_ = null;
  this.isRegistered_ = false;
};

/**
 * Register with the observer service.
 */
activity.ObserverBase.prototype.register = function() {
  if (this.isDisposed()) {
    throw new Error('already disposed');
  }

  this.isRegistered_ = true;
  this.endTimeUsec_ = 0;
  this.events_ = [];

  if (this.observerService_ != null) {
    this.observerService_.addObserver(this, this.observerTopic_, false);
  }
};

/**
 * Unregister with the observer service.
 */
activity.ObserverBase.prototype.unregister = function() {
  if (!this.isRegistered_) {
    return;
  }

  this.isRegistered_ = false;
  // Set the end time to be the first whole resolution greater than
  // the current time, relative to the start time.
  this.endTimeUsec_ = this.roundUpToNearestWholeResolution_(
      this.getCurrentTimeUsec());

  if (this.observerService_ != null) {
    this.observerService_.removeObserver(this, this.observerTopic_);
  }
};

/**
 * Invoked by the TimelineManager when its timer fires.
 * @return {boolean} true if onTimeoutCallback needs to be called
 *     again next time the timer fires, false otherwise.
 */
activity.ObserverBase.prototype.onTimeoutCallback = function() {
  if (this.isDisposed()) {
    return false;
  }

  if (this.hasEvents_()) {
    if (this.isRegistered_) {
      // Continue to extend the end time as long as we're registered.
      this.endTimeUsec_ = this.computeEndTimeUsec_();
    }
    this.timelineModel_.startAddingEvents();
    var unconsumedEvents = [];
    for (var i = 0, len = this.events_.length; i < len; i++) {
      var event = this.events_[i];
      var consumed = this.publishEvent_(event, this.endTimeUsec_);
      if (!consumed) {
        unconsumedEvents.push(event);
      }
    }
    this.events_ = unconsumedEvents;
    this.timelineModel_.doneAddingEvents(this.endTimeUsec_);
  }

  return this.isRegistered_;
};

/**
 * Called by derived classes to add a new event to the event array.
 * @param {string} url the URL for the event.
 * @param {number} timestampUsec the timestamp for the event.
 * @param {activity.TimelineEventType} type type of event.
 * @return {activity.ObserverBase.Event} the new event.
 * @protected
 */
activity.ObserverBase.prototype.addEvent = function(url, timestampUsec, type) {

  var existingEvent = this.getIncompleteEventForUrlAndType(url, type);
  if (existingEvent) {
    // TODO: Make sure all code paths that can get here have a try block to
    // catch and report this error.
    throw new Error([
        'Adding a duplicate event.  Existing event:\n',
        '\t', existingEvent, '\n',
        '\tNew event data: url = ', url, ', type = ', type, '\n'
        ].join(''));
  }

  var event = new activity.ObserverBase.Event(url, timestampUsec, type);

  this.events_.push(event);
  return event;
};

/**
 * @return {number} the start time of the profiling session.
 * @protected
 */
activity.ObserverBase.prototype.getStartTimeUsec = function() {
  return this.startTimeUsec_;
};

/**
 * @return {number} the resolution of our events.
 * @protected
 */
activity.ObserverBase.prototype.getResolutionUsec = function() {
  return this.resolutionUsec_;
};

/**
 * @return {number} the current time, relative to the start time.
 * @protected
 */
activity.ObserverBase.prototype.getCurrentTimeUsec = function() {
  var now = this.currentTimeFactory_.getCurrentTimeUsec();
  return now - this.startTimeUsec_;
};

/**
 * Find the incomplete event with the given URL and type, or null.
 * @param {string} url the URL to search for.
 * @param {activity.TimelineEventType} type the type of event.
 * @return {activity.ObserverBase.Event?} the event, or null.
 */
activity.ObserverBase.prototype.getIncompleteEventForUrlAndType = function(
    url, type) {

  var matchingEvents = [];

  for (var i = 0, len = this.events_.length; i < len; i++) {
    var event = this.events_[i];
    if (!event.isComplete() &&
        event.getUrl() == url &&
        event.getType() == type) {
      matchingEvents.push(event);
    }
  }

  if (matchingEvents.length == 0) {
    return null;
  }

  if (matchingEvents.length > 1) {
    throw new Error([
        'getIncompleteEventForUrlAndType: ',
        matchingEvents.length, ' events match the url ', url,
        ' and type ', type, '.  No more than one event should match.  ',
        'Matching events are:\n', matchingEvents.join('\n')
        ].join(''));
  }

  return matchingEvents[0];
};

/**
 * @param {nsIRequest} request the request to get the URL for.
 * @return {string} the URL for the request.
 * @protected
 */
activity.ObserverBase.prototype.getUrlForRequest = function(request) {
  var url = request.name;
  var hashIndex = url.indexOf('#');
  if (hashIndex >= 0) {
    url = url.substring(0, hashIndex);
  }
  return url;
};

/**
 * @return {boolean} whether this observer has any events.
 * @private
 */
activity.ObserverBase.prototype.hasEvents_ = function() {
  return (this.events_ != null && this.events_.length > 0);
};

/**
 * Compute the end time, based on the state of the active requests and
 * the events.
 * @return {number} the end time, in microseconds.
 * @private
 */
activity.ObserverBase.prototype.computeEndTimeUsec_ = function() {
  var maxTimeUsec =
      this.roundUpToNearestWholeResolution_(this.getCurrentTimeUsec());

  var endTimeUsec = 0;

  // Find the end time for the active requests.
  for (var i = 0, len = this.events_.length; i < len; i++) {
    var event = this.events_[i];
    var candidateEndTime =
        event.isComplete() ? event.getEndTimeUsec() : maxTimeUsec;
    if (candidateEndTime > endTimeUsec) {
      endTimeUsec = candidateEndTime;
    }
  }

  return this.roundUpToNearestWholeResolution_(endTimeUsec);
};

/**
 * Publish the given event.
 * @param {activity.ObserverBase.Event} event the event.
 * @param {number} maxTimeUsec the max time. Events with a timestamp
 *     greater than this value should not be published.
 * @return {boolean} whether the event was published.
 * @private
 */
activity.ObserverBase.prototype.publishEvent_ = function(event, maxTimeUsec) {
  var eventStartUsec = event.getStartTimeUsec();
  if (eventStartUsec > maxTimeUsec) {
    return false;
  }
  var eventEndUsec = event.isComplete() ? event.getEndTimeUsec() : maxTimeUsec;
  if (eventEndUsec > maxTimeUsec) {
    // clamp to the max time.
    eventEndUsec = maxTimeUsec;
  }

  if (eventStartUsec > eventEndUsec) {
    throw new Error([
        'Event start exceeds event end: ',
        '  eventStartUsec =', eventStartUsec,
        '  eventEndUsec =', eventEndUsec,
        '  event = ', event
        ].join(''));
  }
  var durationUsec;
  if (event.isInstantaneous()) {
    // We mark instantaneous events as completely filling the single
    // bucket that they fall into.
    eventStartUsec = this.roundDownToNearestWholeResolution_(eventStartUsec);
    durationUsec = this.resolutionUsec_;
  } else {
    durationUsec = eventEndUsec - eventStartUsec;
  }
    this.timelineModel_.addEvent(
        new activity.TimelineModel.Event(
            event.getUrl(),
            eventStartUsec,
            durationUsec,
            event.getType(),
            1.0));

  return event.isComplete();
};

/**
 * @param {activity.ObserverBase.Event} event the event.
 * @param {number} eventTimeUsec the start time of the period we're
 *     computing intensity for.
 * @param {number} startTimeUsec the start time for the event.
 * @param {number} endTimeUsec the end time for the event.
 * @return {number} the intensity (a value between 0 and 1).
 * @private
 */
activity.ObserverBase.prototype.computeIntensity_ = function(
    event, eventTimeUsec, startTimeUsec, endTimeUsec) {
  // Instantaneous events are marked with 100% intensity for the
  // single bucket they occupy in the UI.
  if (event.isInstantaneous()) return 1.0;

  var duration = this.getResolutionUsec();
  if (eventTimeUsec < startTimeUsec) {
    duration -= (startTimeUsec - eventTimeUsec);
  }
  var bucketEndTimeUsec = eventTimeUsec + this.getResolutionUsec();
  if (bucketEndTimeUsec > endTimeUsec) {
    duration -= (bucketEndTimeUsec - endTimeUsec);
  }
  if (duration <= 0 || duration > this.getResolutionUsec()) {
    throw new Error('event duration out of bounds');
  }
  return duration / this.getResolutionUsec();
};

/**
 * Round the given value down to the nearest whole resolution.
 * @param {number} value the value to round.
 * @return {number} the rounded resolution.
 * @private
 */
activity.ObserverBase.prototype.roundDownToNearestWholeResolution_ = function(
    value) {
  return value - (value % this.resolutionUsec_);
};

/**
 * Round the given value up to the nearest whole resolution.
 * @param {number} value the value to round.
 * @return {number} the rounded resolution.
 * @private
 */
activity.ObserverBase.prototype.roundUpToNearestWholeResolution_ = function(
    value) {
  var mod = value % this.resolutionUsec_;
  if (mod == 0) {
    return value;
  }
  return value + (this.resolutionUsec_ - mod);
};


/**
 * @param {string} url the URL for the request.
 * @param {number} startTimeUsec the start time of the request.
 * @param {activity.TimelineEventType} type the type of event.
 * @constructor
 */
activity.ObserverBase.Event = function(url, startTimeUsec, type) {
  this.url_ = url;
  this.startTimeUsec_ = startTimeUsec;
  this.type_ = type;
  this.endTimeUsec_ = -1;
};

/**
 * @return {string} the URL.
 */
activity.ObserverBase.Event.prototype.getUrl = function() {
  return this.url_;
};

/**
 * @return {number} the start time, in microseconds.
 */
activity.ObserverBase.Event.prototype.getStartTimeUsec = function() {
  return this.startTimeUsec_;
};

/**
 * @return {number} the end time, exclusive, in microseconds.
 */
activity.ObserverBase.Event.prototype.getEndTimeUsec = function() {
  if (!this.isComplete()) {
    throw new Error('end time not available yet.');
  }
  // Add one, since the caller wants an exclusive end time, but we
  // were given an inclusive end time.
  return this.endTimeUsec_ + 1;
};

/**
 * @return {boolean} whether this request is still complete.
 */
activity.ObserverBase.Event.prototype.isComplete = function() {
  return this.endTimeUsec_ != -1;
};

/**
 * @return {boolean} whether this event is an instantaneous event.
 */
activity.ObserverBase.Event.prototype.isInstantaneous = function() {
  return this.startTimeUsec_ == this.endTimeUsec_;
};

/**
 * Indicate that this is an instantaneous event.
 */
activity.ObserverBase.Event.prototype.markAsInstantaneous = function() {
  this.endTimeUsec_ = this.startTimeUsec_;
};

/**
 * Called when a request has completed.
 * @param {number} endTimeUsec the time the event completed, inclusive.
 */
activity.ObserverBase.Event.prototype.onComplete = function(
    endTimeUsec) {
  if (this.endTimeUsec_ != -1) {
    throw new Error([
        'activity.ObserverBase.Event.onComplete(', endTimeUsec, '): ',
        'Event already has end time of ', this.endTimeUsec_, '.  ',
        'event = ', this
        ].join(''));
  }

  this.endTimeUsec_ = endTimeUsec;

  if (this.endTimeUsec_ < this.startTimeUsec_) {
    throw new Error([
        'activity.ObserverBase.Event.onComplete(', endTimeUsec, '): ',
        'Completed an event before it started!  ',
        'event = ', this
        ].join(''));
  }
};

/**
 * @return {activity.TimelineEventType} the type of event.
 */
activity.ObserverBase.Event.prototype.getType = function() {
  return this.type_;
};

/**
 * @return {string} String showing all data held in the event object.
 */
activity.ObserverBase.Event.prototype.toString = function() {
  return JSON.stringify(this, null, '  ');
};
