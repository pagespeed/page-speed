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
 * @fileoverview FirebugProtector prevents firebug from stomping on our
 * handles to the JSD while our profiler is active.
 *
 * @author Bryan McQuade
 */

goog.provide('activity.FirebugProtector');

/**
 * @param {FirebugService} firebugService The firebug service instance,
 *     or null if firebug is not installed.
 * @param {Function} functionPatch The function to patch over non-whitelisted
 *     firebug service functions.
 * @constructor
 */
activity.FirebugProtector = function(firebugService, functionPatch) {
  /**
   * Handle to the firebug service singleton.
   * @type {Object}
   * @private
   */
  this.fbs_ = firebugService;

  /**
   * The function to patch over the non-whitelisted Firebug service functions.
   * @private
   */
  this.functionPatch_ = functionPatch;
};

/**
 * List of Firebug service functions that we don't intercept while profiling.
 * @private
 */
activity.FirebugProtector.prototype.functionWhitelist_ = {
  QueryInterface: true,
  broadcast: true,
  countContext: true,
  findErrorBreakpoint: true,
  hasErrorBreakpoint: true,
  lastErrorWindow: true,
  lockDebugger: true,
  pause: true,
  registerClient: true,
  registerDebugger: true,
  shutdown: true,
  startProfiling: true,
  stopProfiling: true,
  unlockDebugger: true,
  unregisterClient: true,
  unregisterDebugger: true
};

/**
 * Whether or not one of the enable/disable debugger patches was invoked.
 * @type {boolean}
 * @private
 */
activity.FirebugProtector.prototype.debuggerPatchCalled_ = false;

/**
 * Whether or not to enable the firebug debugger when stop() is called.
 * @type {boolean}
 * @private
 */
activity.FirebugProtector.prototype.enableDebugger_ = false;

/**
 * Start the protector. This will patch over non-whitelisted functions
 * to intercept Firebug calls that might stomp on our JSD hooks.
 */
activity.FirebugProtector.prototype.start = function() {
  if (this.fbs_ != null) {
    this.debuggerPatchCalled_ = false;
    this.enableDebugger_ = false;

    // Patch over all non-whitelisted functions while we're profiling.
    for (var prop in this.fbs_) {
      if (typeof this.fbs_[prop] == 'function') {
        if (!this.isWhitelistedFunction_(prop)) {
          this.fbs_[prop] = this.functionPatch_;
        }
      }
    }

    // Apply special patches to the enable/disable debugger API.
    this.fbs_.enableDebugger = goog.bind(this.enableDebuggerPatch_, this);
    this.fbs_.disableDebugger = goog.bind(this.disableDebuggerPatch_, this);
  }
};

/**
 * Stop the protector. This will unpatch any patched functions and
 * update the firebug state accordingly.
 */
activity.FirebugProtector.prototype.stop = function() {
  if (this.fbs_ != null) {
    // First, remove all of our patches.
    for (var prop in this.fbs_) {
      if (this.fbs_[prop] == this.functionPatch_) {
        delete this.fbs_[prop];
      }
    }

    // Remove the special patches as well.
    delete this.fbs_.enableDebugger;
    delete this.fbs_.disableDebugger;

    if (this.debuggerPatchCalled_) {
      // Enable or disable the firebug debugger, based on the patches
      // that got invoked while we were running.
      if (this.enableDebugger_) {
        this.fbs_.enableDebugger();
      } else {
        this.fbs_.disableDebugger();
      }

      this.debuggerPatchCalled_ = false;
      this.enableDebugger_ = false;
    }
  }
};

/**
 * Is the given function name a whitelisted function?
 * @param {string} fname the name of the function.
 * @return {boolean} whether the function is whitelisted.
 * @private
 */
activity.FirebugProtector.prototype.isWhitelistedFunction_ = function(fname) {
  if (this.functionWhitelist_[fname]) return true;
  return false;
};

/**
 * Patch for the firebug service's enableDebugger() function.
 * @private
 */
activity.FirebugProtector.prototype.enableDebuggerPatch_ = function() {
  this.debuggerPatchCalled_ = true;
  this.enableDebugger_ = true;
};

/**
 * Patch for the firebug service's disableDebugger() function.
 * @private
 */
activity.FirebugProtector.prototype.disableDebuggerPatch_ = function() {
  this.debuggerPatchCalled_ = true;
  this.enableDebugger_ = false;
};
