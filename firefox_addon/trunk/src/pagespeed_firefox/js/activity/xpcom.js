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
 * @fileoverview XPCOM component utilities. Based on Firebug's
 * xpcom.js. Allows us to stub out interaction with Components objects
 * when running code in unittests.
 */

goog.provide('activity.xpcom');

/**
 * @param {string} cName the component name.
 * @return {nsISupports?} the component class for the specified
 * component name.
 */
activity.xpcom.CC = function(cName) {
    return Components.classes[cName];
};

/**
 * @param {string} ifaceName the interface name.
 * @return {nsISupports?} the component interface for the specified
 *     interface name.
 */
activity.xpcom.CI = function(ifaceName) {
    return Components.interfaces[ifaceName];
};

/**
 * @param {string} cName the component name.
 * @param {string} ifaceName the interface name.
 * @return {nsISupports?} the service singleton for the specified
 * component name.
 */
activity.xpcom.CCSV = function(cName, ifaceName) {
    return Components.classes[cName].getService(
        Components.interfaces[ifaceName]);
};

/**
 * @param {string} cName the component name.
 * @param {string} ifaceName the interface name.
 * @return {nsISupports?} an instance of the specified component.
 */
activity.xpcom.CCIN = function(cName, ifaceName) {
    return Components.classes[cName].createInstance(
        Components.interfaces[ifaceName]);
};

/**
 * @param {nsISupports} obj the object to query-interface.
 * @param {nsISupports} iface the target interface.
 * @return {nsISupports?} the query-interface result.
 */
activity.xpcom.QI = function(obj, iface) {
    return obj.QueryInterface(iface);
};
