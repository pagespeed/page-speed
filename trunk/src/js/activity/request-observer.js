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
 * @fileoverview Observer of HTTP requests (both cache and network).
 *
 * @author Bryan McQuade
 */

goog.provide('activity.RequestObserver');

goog.require('activity.ObserverBase');
goog.require('activity.TimelineEventType');
goog.require('activity.TimelineModel');
goog.require('activity.TimelineModel.Event');
goog.require('activity.xpcom');

/**
 * Construct a new RequestObserver.
 * @param {Object} timeoutFactory object that provides setTimeout,
 *     clearTimeout (e.g. the global window object).
 * @param {Object} currentTimeFactory object that provides getCurrentTimeUsec.
 * @param {nsIObserverService} observerService the observer service.
 * @param {!activity.TimelineModel} timelineModel the timeline model.
 * @param {number} startTimeUsec the start time of the profiling
 *     session, in microseconds.
 * @param {number} resolutionUsec the resolution, in microseconds.
 * @param {Function} callbackWrapper wrapper function that handles
 *     exceptions thrown by a callback function.
 * @constructor
 * @extends {activity.ObserverBase}
 */
activity.RequestObserver = function(timeoutFactory,
                                    currentTimeFactory,
                                    observerService,
                                    timelineModel,
                                    startTimeUsec,
                                    resolutionUsec,
                                    callbackWrapper) {
  this.observe = goog.partial(
      callbackWrapper,
      goog.bind(activity.RequestObserver.prototype.observe_, this));

  activity.ObserverBase.call(this,
                             currentTimeFactory,
                             observerService,
                             activity.RequestObserver.HTTP_REQUEST_TOPIC_,
                             timelineModel,
                             startTimeUsec,
                             resolutionUsec);

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
};
goog.inherits(activity.RequestObserver, activity.ObserverBase);

/**
 * The preference name to indicate whether data available events
 * should be instantaneous events, or they should show their full
 * start and end times.
 * @type {string}
 * @private
 */
activity.RequestObserver.PREF_INSTANTANEOUS_DATA_AVAILABLE_ =
    'ui_data_available_instantaneous';

/**
 * The default for whether data available should be an instantaneous
 * event.
 * @type {boolean}
 * @private
 */
activity.RequestObserver.DEFAULT_INSTANTANEOUS_DATA_AVAILABLE_ = true;

/**
 * Observer topic that's called for each HTTP request (both cache and
 * non-cache). Note that this topic is *not* called for image requests
 * served directly from the image cache. Timing data for this topic is
 * less accurate for requests issued over the network (since the
 * timings are generated in the UI thread).
 * @type {string}
 * @private
 */
activity.RequestObserver.HTTP_REQUEST_TOPIC_ = 'http-on-modify-request';

/**
 * Whether or not the data available event should be instantaneous.
 * @type {boolean}
 * @private
 */
activity.RequestObserver.prototype.dataAvailableIsInstantaneous_;

/**
 * Register with the observer service.
 */
activity.RequestObserver.prototype.register = function() {
  this.dataAvailableIsInstantaneous_ = activity.preference.getBool(
      activity.RequestObserver.PREF_INSTANTANEOUS_DATA_AVAILABLE_,
      activity.RequestObserver.DEFAULT_INSTANTANEOUS_DATA_AVAILABLE_);

  activity.RequestObserver.superClass_.register.call(this);
};

/**
 * Part of the nsIObserver interface.
 * @param {nsISupports} subject The object being observed.
 * @param {string} topic the topic being observed.
 * @param {string} data the data associated with the observed event.
 * @private
 */
activity.RequestObserver.prototype.observe_ = function(subject, topic, data) {
  if (this.isDisposed()) {
    return;
  }

  if (topic != activity.RequestObserver.HTTP_REQUEST_TOPIC_) {
    return;
  }

  // We're in the http-on-modify-request callback. This callback is
  // invoked right before we check to see if the resource is in the
  // cache. So, we grab the timestamp at this time and pass it to the
  // async callback below, since the current time is the time that
  // we're checking to see if the resource is present in the cache.
  var timestampUsec = this.getCurrentTimeUsec();

  // QueryInterface to nsIRequest, which is the API our callbacks
  // expect.
  var request = activity.xpcom.QI(subject, activity.xpcom.CI('nsIRequest'));

  // Schedule a callback to run on the UI thread immediately. The
  // callback will wire us up to the request object. We have to do
  // this in a callback because any work we do here will get stomped
  // on immedately after we do it (see the implementation of
  // nsHttpRequest::AsyncOpen() for details why this is so).
  var callback = goog.partial(
      this.callbackWrapper_,
      goog.bind(this.attachStreamListener_, this, request, timestampUsec));
  this.timeoutFactory_.setTimeout(callback, 0);
};

/**
 * Callback triggered asynchronously from the observer callback.
 * @param {nsIRequest} request The request to check.
 * @param {number} requestInitTimeUsec the time that the request was
 *     initiated, in microseconds.
 * @private
 */
activity.RequestObserver.prototype.attachStreamListener_ = function(
    request, requestInitTimeUsec) {
  if (this.isDisposed()) {
    return;
  }

  if (!Components.isSuccessCode(request.status)) {
    // The request was aborted. Ignore it.
    return;
  }

  if (activity.RequestObserver.isServingFromCache_(request)) {
    // The resource is being served from cache, so record a cache hit.
    var event = this.addEvent(
        this.getUrlForRequest(request),
        requestInitTimeUsec,
        activity.TimelineEventType.CACHE_HIT);

    // Cache hits are instantaneous events.
    event.markAsInstantaneous();
  }

  // Inject a StreamListener into the listener chain, so we get
  // notified when the data for the request gets passed to the UI
  // thread. We do this here instead of in an http-on-examine-response
  // callback because the latter doesn't get invoked for responses
  // served from the cache.
  var traceableIface = activity.xpcom.CI('nsITraceableChannel');
  if (!(request instanceof traceableIface)) {
    return;
  }

  var traceable = activity.xpcom.QI(request, traceableIface);
  var listener = new activity.RequestObserver.StreamListener_(this);
  try {
    var oldListener = traceable.setNewListener(listener);
    listener.setNext(oldListener);
  } catch (e) {
    // Sometimes we are unable to schedule our async callback quickly
    // enough, and the OnStartRequest for the given request has
    // already fired by the time this code executes. In these cases,
    // setNewListener() will throw an NS_ERROR_FAILURE. There's
    // nothing we can do about it. It means we won't get data
    // available events for that request object. To fix, we would need
    // to re-order the calls in nsHttpChannel::AsyncOpen() so
    // mListener is set before OnModifyRequest gets called, and then
    // make this code run synchronously in the http-on-modify-request
    // callback.
  }
};

/**
 * Invoked when data for the given request was first made available to
 * the UI thread.
 * @param {nsIRequest} request the request.
 */
activity.RequestObserver.prototype.onDataAvailableStart = function(request) {
  if (this.isDisposed()) {
    return;
  }

  var event = this.addEvent(
    this.getUrlForRequest(request),
    this.getCurrentTimeUsec(),
    activity.TimelineEventType.DATA_AVAILABLE);

  if (this.dataAvailableIsInstantaneous_) {
    // Data available events are being handled as instantaneous
    // events.
    event.markAsInstantaneous();
  }
};

/**
 * Invoked when data for the given request was finished being made
 * available to the UI thread.
 * @param {nsIRequest} request the request.
 */
activity.RequestObserver.prototype.onDataAvailableEnd = function(request) {
  if (this.isDisposed()) {
    return;
  }

  if (this.dataAvailableIsInstantaneous_) {
    // Data available events are being handled as instantaneous
    // events, so don't record the end time.
    return;
  }

  var url = this.getUrlForRequest(request);
  var event = this.getIncompleteEventForUrlAndType(
      url, activity.TimelineEventType.DATA_AVAILABLE);
  if (event == null) {
    throw new Error('No such event.');
  }

  event.onComplete(this.getCurrentTimeUsec());
};

/**
 * @param {nsIRequest} request the request to get the URL for.
 * @return {boolean} whether the response is being served from cache.
 * @private
 */
activity.RequestObserver.isServingFromCache_ = function(request) {
  var cachingChannel =
      activity.xpcom.QI(request, activity.xpcom.CI('nsICachingChannel'));
  return request.isPending() && cachingChannel.isFromCache();
};


/**
 * A StreamListener_ gets injected into the response listener chain so
 * we can discover when data is made available to the UI.
 * @param {activity.RequestObserver} observer the associated
 *     RequestObserver.
 * @constructor
 * @private
 */
activity.RequestObserver.StreamListener_ = function(observer) {
  this.observer_ = observer;
  this.next_ = null;
};

/**
 * Set the next stream listener in the chain.
 * @param {nsIStreamListener} next the next listener.
 */
activity.RequestObserver.StreamListener_.prototype.setNext = function(next) {
  this.next_ = next;
};

/**
 * Called when a request is initiated.
 * @param {nsIRequest} request the request.
 * @param {nsISupports} context the context.
 */
activity.RequestObserver.StreamListener_.prototype.onStartRequest = function(
    request, context) {
  if (this.next_)
    this.next_.onStartRequest(request, context);
};

/**
 * Called when a request is completed.
 * @param {nsIRequest} request the request.
 * @param {nsISupports} context the context.
 * @param {number} statusCode the status code.
 */
activity.RequestObserver.StreamListener_.prototype.onStopRequest = function(
    request, context, statusCode) {
  if (this.next_)
    this.next_.onStopRequest(request, context, statusCode);
};

/**
 * Called when data for the request is made available to the UI.
 * @param {nsIRequest} request the request.
 * @param {nsISupports} context the context.
 * @param {nsIInputStream} inputStream the input stream.
 * @param {number} offset the offset in the stream.
 * @param {number} count the amount of data available.
 */
activity.RequestObserver.StreamListener_.prototype.onDataAvailable = function(
    request, context, inputStream, offset, count) {
  this.observer_.onDataAvailableStart(request);

  if (this.next_) {
    this.next_.onDataAvailable(request, context, inputStream, offset, count);
  }

  this.observer_.onDataAvailableEnd(request);
};
