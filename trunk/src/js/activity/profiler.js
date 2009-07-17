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
 * @fileoverview Wrapper around the Firefox JavaScript debugger
 * service (jsdIDebuggerService).
 *
 * @author Bryan McQuade
 */

goog.provide('activity.Profiler');

/**
 * Wrapper around the xpcom internals that enable JavaScript profiling.
 * @constructor
 */
activity.Profiler = function() {
};

/**
 * The interface ID of the version of the jsdIDebuggerService that we
 * currently support. This is the version available up through FF3.0.x.
 * @type {string}
 * @private
 */
activity.Profiler.SUPPORTED_JSD_IID_ = '{9dd9006a-4e5e-4a80-ac3d-007fb7335ca4}';

/**
 * The preference name indicating whether or not we should always
 * treat the JSD as compatible.
 * @type {string}
 * @private
 */
activity.Profiler.PREF_FORCE_COMPATIBLE_JSD_ = 'force_compatible_jsd';

/**
 * @return {boolean} whether or not the version of the
 * jsdIDebuggerService matches the version we expect.
 */
activity.Profiler.isCompatibleJsd = function() {
  if (activity.preference.getBool(
        activity.Profiler.PREF_FORCE_COMPATIBLE_JSD_, false)) {
    return true;
  }
  var jsdIface = activity.xpcom.CI('jsdIDebuggerService');
  if (!jsdIface) return false;
  return jsdIface.number.toLowerCase() == activity.Profiler.SUPPORTED_JSD_IID_;
};

/**
 * Handle to the Firefox JS debugger service singleton instance.
 * @type {jsdIDebuggerService}
 * @private
 */
activity.Profiler.prototype.jsd_ = activity.xpcom.CCSV(
    '@mozilla.org/js/jsd/debugger-service;1', 'jsdIDebuggerService');

/**
 * Handle to the Firefox JS debugger service interface.
 * @type {jsdIDebuggerService}
 * @private
 */
activity.Profiler.prototype.jsdIDebuggerService_ =
    activity.xpcom.CI('jsdIDebuggerService');

/**
 * Start the Firefox JavaScript debugger.
 */
activity.Profiler.prototype.start = function() {
  if (this.jsd_.isOn) {
    // The JSD is already running. Presumably it was started by a
    // different extension (e.g. Firebug). Take a snapshot of the JSD
    // state before we take over, so we can restore the state once
    // we're finished.
    this.jsdSnapshot = {
      breakpointHook: this.jsd_.breakpointHook,
      debuggerHook: this.jsd_.debuggerHook,
      debugHook: this.jsd_.debugHook,
      errorHook: this.jsd_.errorHook,
      functionHook: this.jsd_.functionHook,
      interruptHook: this.jsd_.interruptHook,
      scriptHook: this.jsd_.scriptHook,
      throwHook: this.jsd_.throwHook,
      topLevelHook: this.jsd_.topLevelHook,
      flags: this.jsd_.flags
    };
  } else {
    this.jsd_.on();
    this.jsdSnapshot = null;
  }

  while (this.jsd_.pauseDepth > 0) {
    // Another extension (e.g. Firebug) may have paused the JSD. We
    // need to make sure the JSD is fully unpaused in order to get any
    // callbacks from it.
    this.jsd_.unPause();
  }

  // Reduces the overhead of profiling.
  this.jsd_.flags =
      this.jsdIDebuggerService_.DISABLE_OBJECT_TRACE |
      this.jsdIDebuggerService_.HIDE_DISABLED_FRAMES;

  // Reset the JSD to a clean state.
  this.jsd_.clearFilters();
  this.jsd_.clearAllBreakpoints();
  this.jsd_.breakpointHook = null;
  this.jsd_.debuggerHook = null;
  this.jsd_.debugHook = null;
  this.jsd_.errorHook = null;
  this.jsd_.functionHook = null;
  this.jsd_.interruptHook = null;
  this.jsd_.scriptHook = null;
  this.jsd_.throwHook = null;
  this.jsd_.topLevelHook = null;

  // NOTE: There is a bug in firefox (seems to be specific to 64-bit builds)
  // that causes crashes when dereferencing the program counter to map from
  // pc to line number. This code is only exercised when evaluating
  // jsdIFilters, so we do not enable and jsdIFilters here, and instead
  // enforce the filtering logic in native code (see
  // CallGraphProfile::ShouldIncludeInProfile()).
};

/**
 * Stop the Firefox JavaScript debugger.
 */
activity.Profiler.prototype.stop = function() {
  if (!this.jsd_.isOn) {
    this.jsdSnapshot = null;
    return;
  }
  if (this.jsdSnapshot) {
    // Restore the previous JSD state.
    this.jsd_.breakpointHook = this.jsdSnapshot.breakpointHook;
    this.jsd_.debuggerHook = this.jsdSnapshot.debuggerHook;
    this.jsd_.debugHook = this.jsdSnapshot.debugHook;
    this.jsd_.errorHook = this.jsdSnapshot.errorHook;
    this.jsd_.functionHook = this.jsdSnapshot.functionHook;
    this.jsd_.interruptHook = this.jsdSnapshot.interruptHook;
    this.jsd_.scriptHook = this.jsdSnapshot.scriptHook;
    this.jsd_.throwHook = this.jsdSnapshot.throwHook;
    this.jsd_.topLevelHook = this.jsdSnapshot.topLevelHook;
    this.jsd_.flags = this.jsdSnapshot.flags;
    this.jsdSnapshot = null;
    this.jsd_.clearFilters();
  } else {
    this.jsd_.off();
    this.jsd_.clearFilters();
  }
};
