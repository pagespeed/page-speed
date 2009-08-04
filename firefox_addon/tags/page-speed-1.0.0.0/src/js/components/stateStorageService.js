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

const IStateStorage = Components.interfaces.IStateStorage;
const nsISupports = Components.interfaces.nsISupports;

const CLASS_ID = Components.ID('{7a596ebc-9a29-4f0e-8ad9-58d48eb79369}');
const CLASS_NAME = 'StateStorageService';
const CONTRACT_ID = '@code.google.com/p/page-speed/StateStorageService;1';

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
  if (!aIID.equals(IStateStorage) &&
      !aIID.equals(nsISupports)) {
    throw Components.results.NS_ERROR_NO_INTERFACE;
  }
  return this;
};

/**
 * Factory method for creating a StateStorageService instance.
 */
var StateStorageFactory = {
  createInstance: function(aOuter, aIID) {
    if (aOuter != null)
      throw Components.results.NS_ERROR_NO_AGGREGATION;
    return (new StateStorageService()).QueryInterface(aIID);
  }
};

// nsIModule
var StateStorageModule = {
  registerSelf: function(aCompMgr, aFileSpec, aLocation, aType) {
    aCompMgr = aCompMgr.
        QueryInterface(Components.interfaces.nsIComponentRegistrar);
    aCompMgr.registerFactoryLocation(CLASS_ID, CLASS_NAME,
        CONTRACT_ID, aFileSpec, aLocation, aType);
  },

  unregisterSelf: function(aCompMgr, aLocation, aType) {
    aCompMgr = aCompMgr.
        QueryInterface(Components.interfaces.nsIComponentRegistrar);
    aCompMgr.unregisterFactoryLocation(CLASS_ID, aLocation);
  },

  getClassObject: function(aCompMgr, aCID, aIID) {
    if (!aIID.equals(Components.interfaces.nsIFactory))
      throw Components.results.NS_ERROR_NOT_IMPLEMENTED;

    if (aCID.equals(CLASS_ID))
      return StateStorageFactory;

    throw Components.results.NS_ERROR_NO_INTERFACE;
  },

  canUnload: function(aCompMgr) { return true; }
};

function NSGetModule(aCompMgr, aFileSpec) {
  return StateStorageModule;
}
