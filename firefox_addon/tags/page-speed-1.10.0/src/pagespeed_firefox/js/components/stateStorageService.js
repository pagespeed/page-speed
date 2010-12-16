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
 * @fileoverview Implementation of IStateStorage, which allows us to
 * share state between Firefox windows.
 *
 * @author Bryan McQuade
 */

Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");

var IStateStorageIface = Components.interfaces.IStateStorage;
var nsISupportsIface = Components.interfaces.nsISupports;

/**
 * StateStorageService Constructor.
 * @constructor
 */
function StateStorageService() {
  this.wrappedJSObject = this;

  /**
   * We persist cached objects in this member, which is a hash map
   * from string key to cached object.
   */
  this.cache = {};
}

StateStorageService.prototype.classID =
    Components.ID('{7a596ebc-9a29-4f0e-8ad9-58d48eb79369}');

StateStorageService.prototype.classDescription =
    "Page Speed state storage service";

StateStorageService.prototype.contractID =
    '@code.google.com/p/page-speed/StateStorageService;1';

/**
 * Get a singleton object with the given string name. Multiple calls
 * to this function with the same name parameter are guaranteed to
 * return the same object, even if called from different browser
 * windows.
 */
StateStorageService.prototype.getCachedObject = function(name) {
  if (!this.cache[name]) {
    this.cache[name] = {};
  }
  return this.cache[name];
};

/**
 * Implement nsISupports
 */
StateStorageService.prototype.QueryInterface = function(aIID) {
  if (!aIID.equals(IStateStorageIface) &&
      !aIID.equals(nsISupportsIface)) {
    throw Components.results.NS_ERROR_NO_INTERFACE;
  }
  return this;
};

// XPCOMUtils.generateNSGetFactory was introduced in Mozilla 2 (Firefox 4).
// XPCOMUtils.generateNSGetModule is for Mozilla 1.9.x (Firefox 3).
if (XPCOMUtils.generateNSGetFactory) {
  var NSGetFactory =
      XPCOMUtils.generateNSGetFactory([StateStorageService]);
} else {
  var NSGetModule =
      XPCOMUtils.generateNSGetModule([StateStorageService]);
}
