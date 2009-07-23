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
 * @fileoverview AppStateObserver observes changes in Firefox
 * application state (shutdown) and takes appropriate action based on
 * those states.
 *
 * @author Bryan McQuade
 */

goog.provide('activity.AppStateObserver');

goog.require('activity.xpcom');

/**
 * @constructor
 */
activity.AppStateObserver = function() {
};

/**
 * Xpcom contract id for the observer service.
 * @type {string}
 * @private
 */
activity.AppStateObserver.OBSERVER_SERVICE_CONTRACTID_ =
    '@mozilla.org/observer-service;1';

/**
 * Xpcom interface name for the observer service.
 * @type {string}
 * @private
 */
activity.AppStateObserver.OBSERVER_SERVICE_INTERFACE_NAME_ =
    'nsIObserverService';

/**
 * Observer topic that gets triggered during application shutdown.
 * @type {string}
 * @private
 */
activity.AppStateObserver.QUIT_APPLICATION_TOPIC_ =
    'quit-application-requested';

/**
 * Register with the observer service.
 */
activity.AppStateObserver.prototype.register = function() {
  this.getObserverService_().addObserver(
      this, activity.AppStateObserver.QUIT_APPLICATION_TOPIC_, false);
};

/**
 * Unregister with the observer service.
 */
activity.AppStateObserver.prototype.unregister = function() {
  this.getObserverService_().removeObserver(
      this, activity.AppStateObserver.QUIT_APPLICATION_TOPIC_);
};

/**
 * Part of the nsIObserver interface.
 * @param {nsISupports} subject The object being observed.
 * @param {string} topic the topic being observed.
 * @param {string} data the data associated with the observed event.
 */
activity.AppStateObserver.prototype.observe = function(subject, topic, data) {
  if (topic == activity.AppStateObserver.QUIT_APPLICATION_TOPIC_) {
    activity.ui.performCommand('quitApplication');
  }
};

/**
 * Get a handle to the observer service.
 * @return {nsIObserverService} observer service singleton.
 * @private
 */
activity.AppStateObserver.prototype.getObserverService_ = function() {
  return activity.xpcom.CCSV(
      activity.AppStateObserver.OBSERVER_SERVICE_CONTRACTID_,
      activity.AppStateObserver.OBSERVER_SERVICE_INTERFACE_NAME_);
};
