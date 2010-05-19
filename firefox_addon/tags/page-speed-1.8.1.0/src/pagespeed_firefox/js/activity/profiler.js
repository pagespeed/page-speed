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

goog.require('activity.xpcom');

/**
 * Wrapper around the xpcom internals that enable JavaScript profiling.
 * @constructor
 */
activity.Profiler = function() {
};

/**
 * Handle to the Firefox JS debugger service singleton instance.
 * @type {jsdIDebuggerService}
 * @private
 */
activity.Profiler.jsd_ = activity.xpcom.CCSV(
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
  if (activity.Profiler.jsd_.isOn) {
    // The JSD is already running. Presumably it was started by a
    // different extension (e.g. Firebug). Take a snapshot of the JSD
    // state before we take over, so we can restore the state once
    // we're finished.
    this.jsdSnapshot = {
      breakpointHook: activity.Profiler.jsd_.breakpointHook,
      debuggerHook: activity.Profiler.jsd_.debuggerHook,
      debugHook: activity.Profiler.jsd_.debugHook,
      errorHook: activity.Profiler.jsd_.errorHook,
      functionHook: activity.Profiler.jsd_.functionHook,
      interruptHook: activity.Profiler.jsd_.interruptHook,
      scriptHook: activity.Profiler.jsd_.scriptHook,
      throwHook: activity.Profiler.jsd_.throwHook,
      topLevelHook: activity.Profiler.jsd_.topLevelHook,
      flags: activity.Profiler.jsd_.flags
    };
  } else {
    activity.Profiler.jsd_.on();
    this.jsdSnapshot = null;
  }

  while (activity.Profiler.jsd_.pauseDepth > 0) {
    // Another extension (e.g. Firebug) may have paused the JSD. We
    // need to make sure the JSD is fully unpaused in order to get any
    // callbacks from it.
    activity.Profiler.jsd_.unPause();
  }

  // Reduces the overhead of profiling.
  activity.Profiler.jsd_.flags =
      this.jsdIDebuggerService_.DISABLE_OBJECT_TRACE |
      this.jsdIDebuggerService_.HIDE_DISABLED_FRAMES;

  // Reset the JSD to a clean state.
  activity.Profiler.jsd_.clearFilters();
  activity.Profiler.jsd_.clearAllBreakpoints();
  activity.Profiler.jsd_.breakpointHook = null;
  activity.Profiler.jsd_.debuggerHook = null;
  activity.Profiler.jsd_.debugHook = null;
  activity.Profiler.jsd_.errorHook = null;
  activity.Profiler.jsd_.functionHook = null;
  activity.Profiler.jsd_.interruptHook = null;
  activity.Profiler.jsd_.scriptHook = null;
  activity.Profiler.jsd_.throwHook = null;
  activity.Profiler.jsd_.topLevelHook = null;

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
  if (!activity.Profiler.jsd_.isOn) {
    this.jsdSnapshot = null;
    return;
  }
  if (this.jsdSnapshot) {
    // Restore the previous JSD state.
    activity.Profiler.jsd_.breakpointHook = this.jsdSnapshot.breakpointHook;
    activity.Profiler.jsd_.debuggerHook = this.jsdSnapshot.debuggerHook;
    activity.Profiler.jsd_.debugHook = this.jsdSnapshot.debugHook;
    activity.Profiler.jsd_.errorHook = this.jsdSnapshot.errorHook;
    activity.Profiler.jsd_.functionHook = this.jsdSnapshot.functionHook;
    activity.Profiler.jsd_.interruptHook = this.jsdSnapshot.interruptHook;
    activity.Profiler.jsd_.scriptHook = this.jsdSnapshot.scriptHook;
    activity.Profiler.jsd_.throwHook = this.jsdSnapshot.throwHook;
    activity.Profiler.jsd_.topLevelHook = this.jsdSnapshot.topLevelHook;
    activity.Profiler.jsd_.flags = this.jsdSnapshot.flags;
    this.jsdSnapshot = null;
    activity.Profiler.jsd_.clearFilters();
  } else {
    activity.Profiler.jsd_.off();
    activity.Profiler.jsd_.clearFilters();
  }
};

/**
 * Pause the Firefox JavaScript debugger.
 */
activity.Profiler.pause = function() {
  if (activity.Profiler.jsd_.isOn) {
    activity.Profiler.jsd_.pause();
  }
};

/**
 * Unpause the Firefox JavaScript debugger.
 */
activity.Profiler.unpause = function() {
  if (activity.Profiler.jsd_.isOn) {
    activity.Profiler.jsd_.unPause();
  }
};
