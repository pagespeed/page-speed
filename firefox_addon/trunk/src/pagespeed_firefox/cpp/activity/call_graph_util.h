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
 */

// Author: Bryan McQuade
//
// Call graph and metadata utilities.

#ifndef CALL_GRAPH_UTIL_H_
#define CALL_GRAPH_UTIL_H_

#include <string>

#include "base/basictypes.h"

namespace activity {

class CallGraph;
class CallGraphProfileSnapshot;
class CallGraphTimelineEventSet;
class CallTree;

namespace util {

int64 RoundDownToNearestWholeMultiple(
    int64 value, int64 multiple);

int64 RoundUpToNearestWholeMultiple(
    int64 value, int64 multiple);

/**
 * Helper that determines how much time the current CallTree
 * executed within the given window, including the execution time of
 * its children.
 */
int64 GetTotalExecutionTimeUsec(
    const CallTree &tree,
    int64 start_time_usec,
    int64 end_time_usec);

/**
 * Helper that determines how much time the current CallTree
 * executed within the given window, excluding the execution time of
 * its children.
 */
int64 GetOwnExecutionTimeUsec(
    const CallTree &tree,
    int64 start_time_usec,
    int64 end_time_usec);

/**
 * Populate the function initialization counts for the given
 * CallGraphTimelineEventSet, based on the contents of the given
 * CallGraphProfileSnapshot, for the given time range (which is
 * relative to the profile start time. start_time_usec is inclusive,
 * end_time_usec is exclusive).
 */
void PopulateFunctionInitCounts(
    const CallGraphProfileSnapshot &snapshot,
    CallGraphTimelineEventSet *events,
    int64 start_time_usec,
    int64 end_time_usec);

/**
 * Populate the execution times for the given
 * CallGraphTimelineEventSet, based on the contents of the given
 * CallGraphProfileSnapshot, for the given time range (which is
 * relative to the profile start time. start_time_usec is inclusive,
 * end_time_usec is exclusive).
 */
void PopulateExecutionTimes(
    const CallGraphProfileSnapshot &snapshot,
    CallGraphTimelineEventSet *events,
    int64 start_time_usec,
    int64 end_time_usec);

/**
 * Get the largest timestamp for the fully constructed portion of the
 * call graph.
 */
int64 GetMaxFullyConstructedCallGraphTimeUsec(const CallGraph &call_graph);

/**
 * Convert a numeric time stamp to a pretty-printed string suitable
 * for display in a UI.
 */
void FormatTime(int64 timestamp_usec, std::string *target);

}  // namespace util

}  // namespace activity

#endif  // CALL_GRAPH_UTIL_H_
