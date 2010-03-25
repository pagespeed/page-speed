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
 * @fileoverview Classes responsible for tracking the state of the timelines
 * and notifying listeners when that state changes.
 *
 * @author Bryan McQuade
 */

goog.provide('activity.Listenable');
goog.provide('activity.TimelineEventType');
goog.provide('activity.TimelineModel');
goog.provide('activity.TimelineModel.Event');
goog.provide('activity.TimelineModel.Listener');
goog.provide('activity.TimelineModel.Timeline');
goog.provide('activity.TimelineModel.Timeline.Listener');

goog.require('goog.Disposable');
goog.require('goog.dispose');

/**
 * Listenable is a base class used for objects that other objects can listen to.
 * @constructor
 * @extends {goog.Disposable}
 */
activity.Listenable = function() {
  goog.Disposable.call(this);

  /**
   * The listeners for this TimelineModel.
   * @type {Array}
   * @private
   */
  this.listeners_ = [];
};
goog.inherits(activity.Listenable, goog.Disposable);

/**
 * Add a listener to this Listenable.
 * @param {Object} listener the listener.
 */
activity.Listenable.prototype.addListener = function(listener) {
  this.listeners_.push(listener);
};

/**
 * Remove a listener from this Listenable.
 * @param {Object} listener the listener.
 */
activity.Listenable.prototype.removeListener = function(listener) {
  var listeners = this.listeners_;
  for (var i = 0, len = listeners.length; i < len; i++) {
    if (listeners[i] == listener) {
      listeners.splice(i, 1);
      return;
    }
  }
};

/**
 * Get the listeners array.
 * @return {Array} the listeners.
 * @protected
 */
activity.Listenable.prototype.getListeners = function() {
  return this.listeners_;
};

/**
 * Release our resources.
 */
activity.Listenable.prototype.disposeInternal = function() {
  activity.Listenable.superClass_.disposeInternal.call(this);

  this.listeners_ = null;
};


/**
 * Enumeration of event types dispatched by this model.
 * @enum {number}
 */
activity.TimelineEventType = {
  NETWORK_WAIT: 1,
  DNS_LOOKUP: 2,
  TCP_CONNECTING: 3,
  TCP_CONNECTED: 4,
  REQUEST_SENT: 5,
  CACHE_HIT: 6,
  SOCKET_DATA: 7,
  DATA_AVAILABLE: 8,
  PAINT: 9,
  JS_PARSE: 10,
  JS_EXECUTE: 11
};

/**
 * Get the number of distinct activity.TimelineEventTypes.
 * @return {number} the number of distinct activity.TimelineEventTypes.
 */
activity.TimelineEventType.getNumberOfEventTypes = function() {
  return activity.TimelineEventType.ordinalToEventTypeMap_.length - 1;
};

/**
 * Array, where each index is the ordinal of the TimelineEventType at
 * that index.
 * @type {Array.<activity.TimelineEventType>}
 * @private
 */
activity.TimelineEventType.ordinalToEventTypeMap_ = [
  null,
  activity.TimelineEventType.NETWORK_WAIT,
  activity.TimelineEventType.DNS_LOOKUP,
  activity.TimelineEventType.TCP_CONNECTING,
  activity.TimelineEventType.TCP_CONNECTED,
  activity.TimelineEventType.REQUEST_SENT,
  activity.TimelineEventType.CACHE_HIT,
  activity.TimelineEventType.SOCKET_DATA,
  activity.TimelineEventType.DATA_AVAILABLE,
  activity.TimelineEventType.PAINT,
  activity.TimelineEventType.JS_PARSE,
  activity.TimelineEventType.JS_EXECUTE
];

/**
 * Get the ordinal associated with the given event type.
 * @param {activity.TimelineEventType} eventType the event type.
 * @return {number} The ordinal of the specified TimelineEventType.
 */
activity.TimelineEventType.getOrdinalForEventType = function(eventType) {
  // Simply convert to the corresponding numeric value.
  return Number(eventType);
};

/**
 * Get the event type for the given ordinal.
 * @param {number} ordinal the ordinal.
 * @return {activity.TimelineEventType} the associated event type.
 */
activity.TimelineEventType.getEventTypeForOrdinal = function(ordinal) {
  return activity.TimelineEventType.ordinalToEventTypeMap_[ordinal];
};

/**
 * Is the event type associated with a request (e.g. network event,
 * cache hit event, data available event).
 * @param {activity.TimelineEventType} eventType the event type.
 * @return {boolean} whether or not the eventType is associated with a
 *     request.
 */
activity.TimelineEventType.isRequestEventType = function(eventType) {
  switch (eventType) {
    case activity.TimelineEventType.NETWORK_WAIT:
    case activity.TimelineEventType.DNS_LOOKUP:
    case activity.TimelineEventType.TCP_CONNECTING:
    case activity.TimelineEventType.TCP_CONNECTED:
    case activity.TimelineEventType.REQUEST_SENT:
    case activity.TimelineEventType.CACHE_HIT:
    case activity.TimelineEventType.SOCKET_DATA:
    case activity.TimelineEventType.DATA_AVAILABLE:
    case activity.TimelineEventType.PAINT:
      return true;
    default:
      return false;
  }
};

/**
 * A TimelineModel keeps track of the files where JavaScript code
 * executed, over time. The TimelineModel is composed of multiple
 * Timeline instances, one for each file in which JavaScript code
 * executed. For instance, if an inline script block in index.html
 * executes for 20ms, then code executes in foo.js for 200ms, The
 * TimelineModel will contain two Timeline instances, one for
 * index.html and one for foo.js. Each Timeline instance will keep
 * track of the execution within its associated file.
 * @extends {activity.Listenable}
 * @constructor
 */
activity.TimelineModel = function() {
  activity.Listenable.call(this);

  /**
   * Map from identifier to associated timeline instance.
   * @type {Object}
   * @private
   */
  this.timelineMap_ = {};
};
goog.inherits(activity.TimelineModel, activity.Listenable);

/**
 * The end time of this TimelineModel, in microseconds.
 * @type {number}
 * @private
 */
activity.TimelineModel.prototype.endTimeUsec_ = 0;

/**
 * The pending end time, in microseconds. The actual endTimeUsec_ will
 * get updated with this value once doneAddingEvents() gets called.
 * @type {number}
 * @private
 */
activity.TimelineModel.prototype.pendingEndTimeUsec_ = 0;

/**
 * Indicates whether we are currently adding events
 * (e.g. startAddingEvents() has been called and doneAddingEvents() has
 * not yet been called).
 * @type {number}
 * @private
 */
activity.TimelineModel.prototype.addingEventDepth_ = 0;

/**
 * Call before starting to add a set of events to the model.
 */
activity.TimelineModel.prototype.startAddingEvents = function() {
  this.addingEventDepth_++;
  if (this.addingEventDepth_ == 1) {
    this.pendingEndTimeUsec_ = 0;
  }
};

/**
 * Add an execution event for the specified identifier.
 * @param {activity.TimelineModel.Event} event the event to add.
 */
activity.TimelineModel.prototype.addEvent = function(event) {
  if (this.addingEventDepth_ == 0) {
    throw new Error('Not currently adding events.');
  }
  var timeline = this.getOrCreateTimeline_(event.getIdentifier());
  timeline.addEvent(event);

  var candidateEndTimeUsec = event.getStartTimeUsec() + event.getDurationUsec();
  if (this.pendingEndTimeUsec_ < candidateEndTimeUsec) {
    this.pendingEndTimeUsec_ = candidateEndTimeUsec;
  }
};

/**
 * Call after having added a set of events to the model.
 *
 * @param {number} endTimeUsec end time of the set of events, in microseconds.
 */
activity.TimelineModel.prototype.doneAddingEvents = function(endTimeUsec) {
  this.addingEventDepth_--;

  if (this.addingEventDepth_ < 0) {
    throw new Error('addingEventDepth_ went negative');
  }

  if (this.addingEventDepth_ != 0) {
    // Wait til all event generators have completed.
    return;
  }

  if (endTimeUsec > this.pendingEndTimeUsec_) {
    this.pendingEndTimeUsec_ = endTimeUsec;
  }
  if (this.pendingEndTimeUsec_ > this.endTimeUsec_) {
    this.endTimeUsec_ = this.pendingEndTimeUsec_;
    this.pendingEndTimeUsec_ = 0;
    this.onEndTimeChanged_();
  }
};

/**
 * Release our resources.
 */
activity.TimelineModel.prototype.disposeInternal = function() {
  activity.TimelineModel.superClass_.disposeInternal.call(this);

  for (var id in this.timelineMap_) {
    goog.dispose(this.timelineMap_[id]);
  }
  this.timelineMap_ = null;
};

/**
 * Get the timeline for the specified timeline if one exists already, or
 * create a new one if no timeline exists for the specified identifier.
 * @param {string} identifier identifier for this timeline.
 * @return {!activity.TimelineModel.Timeline} the timeline.
 * @private
 */
activity.TimelineModel.prototype.getOrCreateTimeline_ = function(identifier) {
  if (!this.timelineMap_[identifier]) {
    var timeline = new activity.TimelineModel.Timeline(identifier);
    this.timelineMap_[identifier] = timeline;
    this.onTimelineCreated_(timeline);
  }

  return this.timelineMap_[identifier];
};

/**
 * Tell the listeners that the end time has changed.
 * @private
 */
activity.TimelineModel.prototype.onEndTimeChanged_ = function() {
  var listeners = this.getListeners();
  for (var i = 0, len = listeners.length; i < len; i++) {
    listeners[i].onEndTimeChanged(this.endTimeUsec_);
  }
};

/**
 * Tell the listeners that a new Timeline instance has been created.
 * @param {!activity.TimelineModel.Timeline} timeline the timeline.
 * @private
 */
activity.TimelineModel.prototype.onTimelineCreated_ = function(timeline) {
  var listeners = this.getListeners();
  for (var i = 0, len = listeners.length; i < len; i++) {
    listeners[i].onTimelineCreated(timeline);
  }
};


/**
 * A Timeline keeps track of JavaScript execution for a single file
 * (e.g. index.html or foo.js). A Timeline is made up of multiple
 * Event instances.
 * @param {string} identifier identifier for this timeline.
 * @constructor
 * @extends {activity.Listenable}
 */
activity.TimelineModel.Timeline = function(identifier) {
  activity.Listenable.call(this);

  /**
   * The identifer for this timeline. Usually a URL.
   * @type {string}
   * @private
   */
  this.identifier_ = identifier;
};
goog.inherits(activity.TimelineModel.Timeline, activity.Listenable);

/**
 * Add an event for this Timeline.
 * @param {activity.TimelineModel.Event} event the event to add.
 */
activity.TimelineModel.Timeline.prototype.addEvent = function(event) {
  var listeners = this.getListeners();
  for (var i = 0, len = listeners.length; i < len; i++) {
    listeners[i].onEvent(event);
  }
};

/**
 * Get the identifier for this Timeline. Usually, the identifier is the
 * filename associated with this Timeline.
 * @return {string} the identifier.
 */
activity.TimelineModel.Timeline.prototype.getIdentifier = function() {
  return this.identifier_;
};

/**
 * TimelineModelListeners get notified when the associated TimelineModel
 * instance changes.
 * @constructor
 */
activity.TimelineModel.Listener = function() {
};

/**
 * Notifies the listener that the end time of the TimelineModel has changed.
 * @param {number} endTimeUsec the new end time for this timeline.
 */
activity.TimelineModel.Listener.prototype.onEndTimeChanged = function(
    endTimeUsec) {
};

/**
 * Notifies the listener that a new Timeline instance has been created.
 * @param {!activity.TimelineModel.Timeline} timeline the timeline.
 */
activity.TimelineModel.Listener.prototype.onTimelineCreated =
    function(timeline) {
};

/**
 * TimelineListeners get notified when the associated Timeline instance
 * changes.
 * @constructor
 */
activity.TimelineModel.Timeline.Listener = function() {
};

/**
 * Notifies the listener that a new JavaScript execution event was
 * generated for the associated Timeline.
 * @param {activity.TimelineModel.Event} event the event.
 */
activity.TimelineModel.Timeline.Listener.prototype.onEvent = function(event) {
};

/**
 * An Event encapsulates a single JavaScript execution event
 * (e.g. a start time and a duration).
 * @constructor
 * @param {string} identifier the identifier for this event (usually the URL).
 * @param {number} startTimeUsec start time of the event, in microseconds.
 * @param {number} durationUsec duration of the event, in microseconds.
 * @param {activity.TimelineEventType} eventType the type of event.
 * @param {number} eventIntensity value from 0.0 to 1.0, that
 *     indicates how much of the event duration was consumed by this
 *     event.
 */
activity.TimelineModel.Event = function(identifier,
                                      startTimeUsec,
                                      durationUsec,
                                      eventType,
                                      eventIntensity) {
  /**
   * The identifier (usually the URL) for this event.
   * @type {string}
   * @private
   */
  this.identifier_ = identifier;

  /**
   * The start time of the event, in microseconds.
   * @type {number}
   * @private
   */
  this.startTimeUsec_ = startTimeUsec;

  /**
   * The duration of the event, in microseconds.
   * @type {number}
   * @private
   */
  this.durationUsec_ = durationUsec;

  this.eventType_ = eventType;

  /**
   * A value from 0.0 to 1.0, that indicates how much of the event
   * duration was consumed by this event.
   * @type {number}
   * @private
   */
  this.eventIntensity_ = eventIntensity;
};

/**
 * @return {string} the identifier.
 */
activity.TimelineModel.Event.prototype.getIdentifier = function() {
  return this.identifier_;
};

/**
 * @return {number} the start time, in microseconds.
 */
activity.TimelineModel.Event.prototype.getStartTimeUsec = function() {
  return this.startTimeUsec_;
};

/**
 * @return {number} the duration, in microseconds.
 */
activity.TimelineModel.Event.prototype.getDurationUsec = function() {
  return this.durationUsec_;
};

/**
 * @return {activity.TimelineEventType} the type of event.
 */
activity.TimelineModel.Event.prototype.getType = function() {
  return this.eventType_;
};

/**
 * @return {number} value from 0.0 to 1.0, that indicates how much of
 *     the event duration was consumed by this event.
 */
activity.TimelineModel.Event.prototype.getIntensity = function() {
  return this.eventIntensity_;
};

/**
 * @param {number} adjustment amount to adjust the intensity by.
 */
activity.TimelineModel.Event.prototype.adjustIntensity = function(adjustment) {
  this.eventIntensity_ += adjustment;
};
