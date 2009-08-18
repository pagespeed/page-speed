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
 * @fileoverview Observer of MozAfterPaint events. Note that these
 * events are new to Firefox 3.5, so this class will not generate any
 * events for versions of Firefox pre-3.5.
 *
 * @author Bryan McQuade
 */

goog.provide('activity.PaintObserver');

goog.require('activity.ObserverBase');
goog.require('activity.PaintView');
goog.require('activity.TimelineEventType');
goog.require('activity.TimelineModel');
goog.require('activity.TimelineModel.Event');

/**
 * Construct a new PaintObserver.
 * @param {Object} currentTimeFactory object that provides getCurrentTimeUsec.
 * @param {nsIDOMDocument} xulElementFactory A factory for creating DOM
 *     elements (e.g. the document object).
 * @param {!activity.TimelineModel} timelineModel the timeline model.
 * @param {activity.PaintView} paintView the paint view.
 * @param {Object} tabBrowser The tabbrowser instance.
 * @param {number} startTimeUsec the start time of the profiling
 *     session, in microseconds.
 * @param {number} resolutionUsec the resolution, in microseconds.
 * @param {Function} callbackWrapper a function to wrap all async
 *     callbacks in (catches exceptions and displays them to the user).
 * @constructor
 * @extends {activity.ObserverBase}
 */
activity.PaintObserver = function(currentTimeFactory,
                                  xulElementFactory,
                                  timelineModel,
                                  paintView,
                                  tabBrowser,
                                  startTimeUsec,
                                  resolutionUsec,
                                  callbackWrapper) {
  // TODO: refactor ObserverBase into a simple class with a
  // register/unregister API, and have ObserverBase derive from that
  // class to provide nsIObserverService hooks. PaintObserver doesn't
  // want the nsIObserverService stuff, but we derive from it for the
  // register/unregister API and state handling.
  activity.ObserverBase.call(this,
                             currentTimeFactory,
                             null,  // we don't use the nsIObserverService
                             '',    // we don't use the nsIObserverService
                             timelineModel,
                             startTimeUsec,
                             resolutionUsec);

  /**
   * Function to wrap all async callbacks in (catches exceptions and
   * displays them to the user).
   * @type {Function}
   * @private
   */
  this.callbackWrapper_ = callbackWrapper;

  /**
   * @type {nsIDOMDocument}
   * @private
   */
  this.xulElementFactory_ = xulElementFactory;

  /**
   * The paint view we should dispatch paint events to.
   * @type {activity.PaintView}
   * @private
   */
  this.paintView_ = paintView;

  /**
   * The tabbrowser object.
   * @type {Object}
   * @private
   */
  this.tabBrowser_ = tabBrowser;

  /**
   * Array of active PaintObserver.Listeners
   * @type {Array.<activity.PaintObserver.Listener_>}
   * @private
   */
  this.paintListeners_ = null;
};
goog.inherits(activity.PaintObserver, activity.ObserverBase);

/**
 * Event name for mozilla paint events.
 * @type {string}
 * @private
 */
activity.PaintObserver.PAINT_EVENT_NAME_ = 'MozAfterPaint';

/**
 * @return {boolean} Whether paint events are supported in the current
 *     version of Firefox.
 */
activity.PaintObserver.prototype.isSupported = function() {
  // We depend on the MozAfterPaint event, which was added in
  // FF3.5. We know that addTabsProgressListener was also added in
  // FF3.5. We aren't aware of a way to ask an event source if it
  // dispatches events of a given name, so we just check for the
  // presence of the addTabsProgressListener method on the tabbrowser
  // object.
  return this.tabBrowser_.addTabsProgressListener != null;
};

/**
 * Register for progress events.
 * @override
 */
activity.PaintObserver.prototype.register = function() {
  if (!this.isSupported()) {
    return;
  }
  if (this.isRegistered()) {
    return;
  }

  activity.PaintObserver.superClass_.register.call(this);

  this.paintListeners_ = [];

  // The MozAfterPaint event dispatcher will only send us cross-origin
  // paint events (e.g. events for a child frame on a different
  // origin) if we are registered as a listener on a window hosting a
  // chrome: URL. So we bind to the parent window of the tabbrowser
  // object, which is a chrome: URL.
  this.registerPaintListener(new activity.PaintObserver.Listener_(
      this, this.tabBrowser_.ownerDocument.defaultView,
      this.callbackWrapper_));
};

/**
 * Unregister for progress events.
 * @override
 */
activity.PaintObserver.prototype.unregister = function() {
  if (!this.isRegistered()) {
    return;
  }

  activity.PaintObserver.superClass_.unregister.call(this);

  // Have to iterate in reverse order, since unregisterPaintListener
  // removes the listener from the array.
  for (var i = this.paintListeners_.length - 1; i >= 0; i--) {
    this.unregisterPaintListener(this.paintListeners_[i]);
  }
  if (this.paintListeners_.length > 0) {
    throw new Error('failed to unregister some listeners');
  }
  this.paintListeners_ = null;
};

/**
 * Add a new paint event for the specified URL, at the current time.
 * @param {MozAfterPaintEvent} event The MozAfterPaint event.
 */
activity.PaintObserver.prototype.onPaintEvent = function(event) {
  var win = event.target;
  if (win.closed || !win.location) {
    return;
  }

  var protocol = win.location.protocol;
  if (protocol != 'http:' && protocol != 'https:') {
    // Only track HTTP(S) events.
    return;
  }

  var url = win.location.href;
  var hashIndex = url.indexOf('#');
  if (hashIndex >= 0) {
    url = url.substring(0, hashIndex);
  }

  var timestampUsec = this.getCurrentTimeUsec();
  var evt = this.addEvent(
      url, timestampUsec, activity.TimelineEventType.PAINT);
  evt.markAsInstantaneous();

  if (this.paintView_) {
    // Dispatch the event to the PaintView which will render a snapshot
    // of the screen to the paint panel. The PaintView will be null if
    // capturing of screenshots is disabled.
    this.paintView_.onPaintEvent(event);
  }
};

/**
 * Add and register the specified paint listener.
 * @param {activity.PaintObserver.Listener} listener the listener to add.
 */
activity.PaintObserver.prototype.registerPaintListener = function(listener) {
  listener.register();
  this.paintListeners_.push(listener);
};

/**
 * Remove and unregister the specified paint listener.
 * @param {activity.PaintObserver.Listener} listener the listener to remove.
 */
activity.PaintObserver.prototype.unregisterPaintListener = function(listener) {
  if (!this.paintListeners_) {
    return;
  }
  var listeners = this.paintListeners_;
  for (var i = 0, len = listeners.length; i < len; i++) {
    if (listeners[i] == listener) {
      listeners.splice(i, 1);
      listener.unregister();
      return;
    }
  }
};

/**
 * @param {activity.PaintObserver} observer The paint observer for this
 *     Listener.
 * @param {nsIDOMWindow} win The window to listen to.
 * @param {Function} callbackWrapper wrapper function to invoke async
 *     callbacks from.
 * @constructor
 * @private
 */
activity.PaintObserver.Listener_ = function(observer, win, callbackWrapper) {
  this.observer_ = observer;
  this.win_ = win;

  this.boundEventCallback_ = goog.partial(
      callbackWrapper,
      goog.bind(this.observer_.onPaintEvent, this.observer_));
  this.boundUnloadCallback_ = goog.partial(
      callbackWrapper,
      goog.bind(this.observer_.unregisterPaintListener, this.observer_));
};

/**
 * Whether or not we're registered for MozAfterPaint events.
 * @type {boolean}
 * @private
 */
activity.PaintObserver.Listener_.prototype.isRegistered_ = false;

/**
 * Register for MozAfterPaint events.
 */
activity.PaintObserver.Listener_.prototype.register = function() {
  if (this.isRegistered_) {
    return;
  }
  this.win_.addEventListener('unload', this.boundUnloadCallback_, false);
  this.win_.addEventListener(activity.PaintObserver.PAINT_EVENT_NAME_,
                             this.boundEventCallback_,
                             false);
  this.isRegistered_ = true;
};

/**
 * Unregister for MozAfterPaint events.
 */
activity.PaintObserver.Listener_.prototype.unregister = function() {
  if (!this.isRegistered_) {
    return;
  }
  this.win_.removeEventListener('unload', this, false);
  this.win_.removeEventListener(activity.PaintObserver.PAINT_EVENT_NAME_,
                                this.boundEventCallback_,
                                false);
  this.isRegistered_ = false;
};
