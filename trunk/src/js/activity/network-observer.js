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
 * @fileoverview Observer of HTTP network events. NetworkObserver gets
 * called when a network request is created and when it gets
 * terminated (among other events).
 *
 * @author Bryan McQuade
 */

goog.provide('activity.NetworkObserver');

goog.require('activity.ObserverBase');
goog.require('activity.TimelineEventType');
goog.require('activity.TimelineModel');
goog.require('activity.TimelineModel.Event');
goog.require('activity.xpcom');

/**
 * Construct a new NetworkObserver.
 * @param {Object} currentTimeFactory object that provides getCurrentTimeUsec.
 * @param {nsIObserverService} observerService the observer service.
 * @param {!activity.TimelineModel} timelineModel the timeline model.
 * @param {number} startTimeUsec the start time of the profiling
 *     session, in microseconds.
 * @param {number} resolutionUsec the resolution, in microseconds.
 * @constructor
 * @extends {activity.ObserverBase}
 */
activity.NetworkObserver = function(currentTimeFactory,
                                  observerService,
                                  timelineModel,
                                  startTimeUsec,
                                  resolutionUsec) {
  activity.ObserverBase.call(this,
                           currentTimeFactory,
                           observerService,
                           activity.NetworkObserver.NETWORK_ACTIVITY_TOPIC_,
                           timelineModel,
                           startTimeUsec,
                           resolutionUsec);
};
goog.inherits(activity.NetworkObserver, activity.ObserverBase);

/**
 * Observer topic name for HTTP activity. This topic is only called
 * for actual network requests (not for responses served from
 * cache). However, the timing data for this topic is more accurate
 * than nsIWebProgressListener, since the timing data for this topic
 * is generated in the network thread.
 * @type {string}
 * @private
 */
activity.NetworkObserver.NETWORK_ACTIVITY_TOPIC_ = 'http-activity-observer';

/**
 * Part of the nsIHttpActivityObserver implementation. Tells the HTTP
 * activity distributor whether or not we are interested in receiving
 * events. It is preferable to leave this value as always true, and
 * register() when interested, and unregister() when no longer
 * interested in receving events.
 * @type {boolean}
 */
activity.NetworkObserver.prototype.isActive = true;

/**
 * Implement nsISupports::QueryInterface.
 * @param {nsIJSID} iid The interface ID to QI on.
 * @return {Object} the result of the QI.
 */
activity.NetworkObserver.prototype.QueryInterface = function(iid) {
  if (!iid.equals(activity.xpcom.CI('nsISupports')) &&
      !iid.equals(activity.xpcom.CI('nsIHttpActivityObserver'))) {
    throw Components.results.NS_ERROR_NO_INTERFACE;
  }
  return this;
};

/**
 * Invoked on the main thread when network activity occurs.
 * @param {nsISupports} httpChannel An nsISupports-derived instance
 *     that should also implement the {nsIHttpChannel} interface.
 * @param {number} activityType enum value that indicates whether this
 *     is a socket-level event or an HTTP transaction-level event.
 * @param {number} activitySubtype enum value that indicates the type
 *     of event.
 * @param {number} timestampUsec the time that this event occurred in the
 *     network thread, in microseconds since epoch.
 * @param {number} extraSizeData any extra size data optionally
 *     available with this event.
 * @param {string} extraStringData any extra string data optionally
 *     available with this event.
 */
activity.NetworkObserver.prototype.observeActivity = function(
    httpChannel,
    activityType,
    activitySubtype,
    timestampUsec,
    extraSizeData,
    extraStringData) {
  if (this.isDisposed()) {
    return;
  }

  // Make the timestamp relative to the start time.
  timestampUsec -= this.getStartTimeUsec();

  /* @type {nsIRequest} */
  var request = activity.xpcom.QI(httpChannel, activity.xpcom.CI('nsIRequest'));

  // Get a handle to the xpcom interface object for
  // nsIHttpActivityObserver.
  var hao = activity.xpcom.CI('nsIHttpActivityObserver');
  if (activityType == hao.ACTIVITY_TYPE_HTTP_TRANSACTION) {
    this.onHttpTransactionEvent(request, activitySubtype, timestampUsec);
  } else if (activityType == hao.ACTIVITY_TYPE_SOCKET_TRANSPORT) {
    this.onSocketTransportEvent(request, activitySubtype, timestampUsec);
  }
};

/**
 * @param {nsIRequest} request The request object associated with the
 *     event.
 * @param {number} activitySubtype enum value that indicates the type
 *     of event.
 * @param {number} timestampUsec the time that this event occurred in the
 *     network thread, in microseconds since epoch.
 */
activity.NetworkObserver.prototype.onHttpTransactionEvent = function(
    request, activitySubtype, timestampUsec) {
  var hao = activity.xpcom.CI('nsIHttpActivityObserver');
  var url = this.getUrlForRequest(request);
  if (activitySubtype == hao.ACTIVITY_SUBTYPE_REQUEST_HEADER) {
    this.addEvent(url, timestampUsec, activity.TimelineEventType.NETWORK_WAIT);
  } else if (activitySubtype == hao.ACTIVITY_SUBTYPE_TRANSACTION_CLOSE) {
    this.maybeCompletePreviousEvent_(
        url, activity.TimelineEventType.SOCKET_DATA, timestampUsec);

    var event = this.getIncompleteEventForUrlAndType(
        url, activity.TimelineEventType.TCP_CONNECTED);
    if (event != null) {
      event.onComplete(timestampUsec);
    }
  }
};

/**
 * @param {nsIRequest} request The request object associated with the
 *     event.
 * @param {number} activitySubtype enum value that indicates the type
 *     of event.
 * @param {number} timestampUsec the time that this event occurred in the
 *     network thread, in microseconds since epoch.
 */
activity.NetworkObserver.prototype.onSocketTransportEvent = function(
    request, activitySubtype, timestampUsec) {
  var url = this.getUrlForRequest(request);
  var timelineEventType = this.getEventTypeForStatus_(activitySubtype);
  if (timelineEventType == null) {
    return;
  }

  this.maybeCompletePreviousEvent_(url, timelineEventType, timestampUsec);

  if (timelineEventType == activity.TimelineEventType.TCP_CONNECTED) {
    var event = this.getIncompleteEventForUrlAndType(
        url, activity.TimelineEventType.TCP_CONNECTING);
    event.onComplete(timestampUsec);
  }

  if (timelineEventType == activity.TimelineEventType.REQUEST_SENT &&
    this.getIncompleteEventForUrlAndType(
        url, activity.TimelineEventType.TCP_CONNECTED) == null) {
    this.addEvent(url, timestampUsec, activity.TimelineEventType.TCP_CONNECTED);
  }

  var event = this.addEvent(url, timestampUsec, timelineEventType);
  // TODO: if FF3.1, don't make REQUEST_SENT instantaneous.
  if (timelineEventType == activity.TimelineEventType.SOCKET_DATA ||
      timelineEventType == activity.TimelineEventType.REQUEST_SENT) {
    event.markAsInstantaneous();
  }
};

/**
 * @param {number} status The status code of the network transport
 *     event.
 * @return {activity.TimelineEventType?} The event type for the given
 *     status.
 * @private
 */
activity.NetworkObserver.prototype.getEventTypeForStatus_ = function(status) {
  var st = activity.xpcom.CI('nsISocketTransport');
  switch (status) {
    case st.STATUS_RESOLVING:
      return activity.TimelineEventType.DNS_LOOKUP;
    case st.STATUS_CONNECTING_TO:
      return activity.TimelineEventType.TCP_CONNECTING;
    case st.STATUS_CONNECTED_TO:
      return activity.TimelineEventType.TCP_CONNECTED;
    case st.STATUS_SENDING_TO:
      return activity.TimelineEventType.REQUEST_SENT;
    case st.STATUS_RECEIVING_FROM:
      return activity.TimelineEventType.SOCKET_DATA;
    default:
      return null;
  }
};

/**
 * @param {string} url The URL of the request associated with the
 *     event.
 * @param {activity.TimelineEventType} timelineEventType The event type.
 * @param {number} timestampUsec the time that this event occurred in the
 *     network thread, in microseconds since epoch.
 * @private
 */
activity.NetworkObserver.prototype.maybeCompletePreviousEvent_ = function(
    url, timelineEventType, timestampUsec) {
  for (var t = activity.TimelineEventType.NETWORK_WAIT;
       t < timelineEventType;
       t++) {
    if (t == activity.TimelineEventType.TCP_CONNECTING ||
        t == activity.TimelineEventType.TCP_CONNECTED) {
      continue;
    }
    var prevEvent = this.getIncompleteEventForUrlAndType(url, t);
    if (prevEvent != null) {
      prevEvent.onComplete(timestampUsec);
    }
  }
};
