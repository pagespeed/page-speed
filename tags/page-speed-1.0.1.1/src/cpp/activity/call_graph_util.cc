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

#include "call_graph_util.h"

#include <algorithm>
#include <limits>

#include "call_graph.h"
#include "call_graph_metadata.h"
#include "call_graph_profile_snapshot.h"
#include "call_graph_timeline_event.h"
#include "call_graph_timeline_event_set.h"
#include "call_graph_timeline_visitor.h"
#include "call_graph_visitor_interface.h"
#include "call_graph_visit_filter_interface.h"
#include "profile.pb.h"
#include "check.h"

#ifdef _WINDOWS

// Provide LONG_LONG_MAX and snprintf, which are available under
// different names on windows.

#include <stdarg.h>  // for va_list and related operations
#include <stdlib.h>  // for _TRUNCATE

#define LONG_LONG_MAX _I64_MAX

namespace {

int snprintf(char *str, size_t size, const char *format, ...) {
  va_list ap;
  va_start(ap, format);
  int result = _vsnprintf_s(str, size, _TRUNCATE, format, ap);
  va_end(ap);

  return result;
}

}  // namespace

#endif  // _WINDOWS

namespace activity {

namespace util {

int64_t RoundDownToNearestWholeMultiple(
    int64_t value, int64_t multiple) {
  GCHECK_GE(value, 0LL);
  GCHECK_GT(multiple, 0LL);
  return value - (value % multiple);
}

int64_t RoundUpToNearestWholeMultiple(
    int64_t value, int64_t multiple) {
  GCHECK_GE(value, 0LL);
  GCHECK_GT(multiple, 0LL);

  const int64_t mod = value % multiple;
  if (mod == 0) {
    return value;
  }

  const int64_t mod_complement = multiple - mod;
  if (LONG_LONG_MAX - mod_complement < value) {
    // Rounding up would cause overflow. The best we can do without
    // overflowing, while returning a value that is a multiple of
    // multiple, is to round down.
    return value - mod;
  }

  const int64_t result = value + mod_complement;
  GCHECK_GE(result, 0LL);
  return result;
}

int64_t GetTotalExecutionTimeUsec(
    const CallTree &tree,
    int64_t start_time_usec,
    int64_t end_time_usec) {
  GCHECK_GE(start_time_usec, 0LL);
  GCHECK_GE(end_time_usec, start_time_usec);

  if (tree.entry_time_usec() >= end_time_usec) {
    return 0LL;
  }
  if (tree.exit_time_usec() <= start_time_usec) {
    return 0LL;
  }

  const int64_t clamped_start_time_usec =
      std::max(int64_t(tree.entry_time_usec()), start_time_usec);
  const int64_t clamped_end_time_usec =
      std::min(int64_t(tree.exit_time_usec()), end_time_usec);

  const int64_t execution_time_usec =
      clamped_end_time_usec - clamped_start_time_usec;

  // Make sure the execution time falls between 0 and the duration.
  GCHECK_GE(execution_time_usec, 0LL);
  GCHECK_GE(end_time_usec - start_time_usec, execution_time_usec);

  return execution_time_usec;
}

int64_t GetOwnExecutionTimeUsec(
    const CallTree &tree,
    int64_t start_time_usec,
    int64_t end_time_usec) {
  GCHECK_GE(start_time_usec, 0LL);
  GCHECK_GE(end_time_usec, start_time_usec);

  // First compute the total execution time for this node.
  int64_t execution_time_usec =
      GetTotalExecutionTimeUsec(tree, start_time_usec, end_time_usec);

  if (execution_time_usec == 0LL) {
    return 0LL;
  }

  // Next subtract the total execution times for each child node.
  for (int i = 0, size = tree.children_size(); i < size; i++) {
    const CallTree& child = tree.children(i);
    execution_time_usec -=
        GetTotalExecutionTimeUsec(
            child, start_time_usec, end_time_usec);
  }

  // Make sure the execution time falls between 0 and the duration.
  GCHECK_GE(execution_time_usec, 0LL);
  GCHECK_GE(end_time_usec - start_time_usec, execution_time_usec);

  return execution_time_usec;
}

void PopulateFunctionInitCounts(
    const CallGraphProfileSnapshot &snapshot,
    CallGraphTimelineEventSet *events,
    int64_t start_time_usec,
    int64_t end_time_usec) {
  GCHECK_GE(start_time_usec, 0LL);
  GCHECK_GE(end_time_usec, 0LL);
  GCHECK_GE(end_time_usec, start_time_usec);

  const CallGraphProfileSnapshot::InitTimeMap *init_time_map =
      snapshot.init_time_map();
  for (CallGraphProfileSnapshot::InitTimeMap::const_iterator
           it = init_time_map->lower_bound(start_time_usec),
           end = init_time_map->lower_bound(end_time_usec);
       it != end;
       ++it) {
    const FunctionMetadata *metadata = it->second;
    const int64_t function_instantiation_time_usec =
        RoundDownToNearestWholeMultiple(
            metadata->function_instantiation_time_usec(),
            events->event_duration_usec());

    CallGraphTimelineEvent *event = events->GetOrCreateEvent(
        metadata->file_name().c_str(),
        CallGraphTimelineEvent::JS_PARSE,
        function_instantiation_time_usec);
    event->intensity++;
  }
}

void PopulateExecutionTimes(
    const CallGraphProfileSnapshot &snapshot,
    CallGraphTimelineEventSet *events,
    int64_t start_time_usec,
    int64_t end_time_usec) {
  GCHECK_GE(start_time_usec, 0LL);
  GCHECK_GE(end_time_usec, 0LL);
  GCHECK_EQ(0LL, start_time_usec % events->event_duration_usec());
  if (end_time_usec != LONG_LONG_MAX) {
    GCHECK_EQ(0LL, end_time_usec % events->event_duration_usec());
  }
  GCHECK_GE(start_time_usec, 0LL);
  GCHECK_GE(end_time_usec, start_time_usec);
  GCHECK(events != NULL);

  CallGraphTimelineVisitor visitor(
      new TimeRangeVisitFilter(start_time_usec, end_time_usec),
      *snapshot.metadata(),
      events,
      start_time_usec,
      end_time_usec);

  snapshot.call_graph()->Traverse(&visitor);
}

int64_t GetMaxFullyConstructedCallGraphTimeUsec(const CallGraph &call_graph) {
  int64_t max_time_usec = 0LL;
  const CallGraph::CallForest *forest = call_graph.call_forest();
  if (!forest->empty()) {
    max_time_usec = forest->back()->exit_time_usec();
  }
  return max_time_usec;
}

void FormatTime(int64_t time_usec, std::string *target) {
  // Very simple format for now: if under 10 seconds, show
  // milliseconds. Otherwise, truncate to whole seconds.
  const long long msec = time_usec / 1000LL;

  const size_t buf_size = 20;
  char tmp[buf_size];
  if (msec < 10000LL) {
    snprintf(tmp, buf_size, "%lld ms", msec);
  } else {
    snprintf(tmp, buf_size, "%lld seconds", (msec / 1000LL));
  }

  target->append(tmp, buf_size);
}

}  // namespace util

}  // namespace activity
