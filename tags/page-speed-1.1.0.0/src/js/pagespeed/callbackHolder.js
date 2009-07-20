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
 * @fileoverview Define an object which holds a collection of callbacks.
 *
 * Sometimes callbacks need to have data passed to them.  The call to
 * invoke the callback will take an object, and that object will be passed
 * to each callback.  Callbacks should not modify the object.
 *
 * Usage:
 *
 * NAMESPACE.fooDoneCallbackHolder = new CallbackHolder();
 *
 * NAMESPACE.fooDoneCallbackHolder.addCallback(function(data) {
 *   ...
 *   alert('Foo's result was ' + data.fooResult + '.');
 *   ...
 * });
 *
 * NAMESPACE.foo = function() {
 *   ...
 *   var fooResult = ...;
 *   ...
 *
 *   // Done with foo(), run post-foo callbacks.
 *   NAMESPACE.fooDoneCallbackHolder.execCallbacks({fooResult: fooResult});
 * };
 *
 * TODO: Add a mechanism to remove a specific callback.
 *
 * @author Sam Kerner
 */

(function() {  // Begin closure

/**
 * Create a new callback holder.
 * @constructor
 */
PAGESPEED.CallbackHolder = function() {
  this.callbacks_ = [];
};

/**
 * Add a callback.
 * @param {Function.Object} callback The callback to add.  May take
 *    a single argument:  An object that holds data passed from the
 *    callback site.
 */
PAGESPEED.CallbackHolder.prototype.addCallback = function(callback) {
  this.callbacks_.push(callback);
};

/**
 * Run all callbacks.
 * @param {Object} data The argument is used to pass data to each callback.
 * @return {Array} The return value of each callback.
 */
PAGESPEED.CallbackHolder.prototype.execCallbacks = function(data) {
  var callbackResults = [];
  for (var i = 0, len = this.callbacks_.length; i < len; ++i) {
    callbackResults.push(this.callbacks_[i](data));
  }
  return callbackResults;
};

/**
 * @return {number} The number of callbacks.
 */
PAGESPEED.CallbackHolder.prototype.getNumCallbacks = function() {
  return this.callbacks_.length;
};

})();  // End closure
