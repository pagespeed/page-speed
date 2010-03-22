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

#include "base/logging.h"
#include "call_graph.h"
#include "call_graph_metadata.h"
#include "call_graph_profile_snapshot.h"
#include "call_graph_timeline_event.h"
#include "call_graph_timeline_event_set.h"
#include "call_graph_timeline_visitor.h"
#include "call_graph_visitor_interface.h"
#include "call_graph_visit_filter_interface.h"
#include "profile.pb.h"

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

int64 RoundDownToNearestWholeMultiple(
    int64 value, int64 multiple) {
  if (value < 0LL || multiple <= 0LL) {
    LOG(DFATAL) << "Bad inputs to RoundDownToNearestWholeMultiple: "
                << value << ", " << multiple;
    return 0LL;
  }
  return value - (value % multiple);
}

int64 RoundUpToNearestWholeMultiple(
    int64 value, int64 multiple) {
  if (value < 0LL || multiple <= 0LL) {
    LOG(DFATAL) << "Bad inputs to RoundUpToNearestWholeMultiple: "
                << value << ", " << multiple;
    return 0LL;
  }

  const int64 mod = value % multiple;
  if (mod == 0) {
    return value;
  }

  const int64 mod_complement = multiple - mod;
  if (LONG_LONG_MAX - mod_complement < value) {
    // Rounding up would cause overflow. The best we can do without
    // overflowing, while returning a value that is a multiple of
    // multiple, is to round down.
    return value - mod;
  }

  const int64 result = value + mod_complement;
  if (result < 0LL) {
    LOG(DFATAL) << "Bad result: " << result;
    return 0LL;
  }
  return result;
}

int64 GetTotalExecutionTimeUsec(
    const CallTree &tree,
    int64 start_time_usec,
    int64 end_time_usec) {
  if (start_time_usec < 0LL) {
    LOG(DFATAL) << "Bad start_time_usec: " << start_time_usec;
    return 0LL;
  }
  if (end_time_usec < start_time_usec) {
    LOG(DFATAL) << "end_time_usec lt start_time_usec: "
                << end_time_usec << " < " << start_time_usec;
    return 0LL;
  }

  if (tree.entry_time_usec() >= end_time_usec) {
    return 0LL;
  }
  if (tree.exit_time_usec() <= start_time_usec) {
    return 0LL;
  }

  const int64 clamped_start_time_usec =
      std::max(int64(tree.entry_time_usec()), start_time_usec);
  const int64 clamped_end_time_usec =
      std::min(int64(tree.exit_time_usec()), end_time_usec);

  const int64 execution_time_usec =
      clamped_end_time_usec - clamped_start_time_usec;

  // Make sure the execution time falls between 0 and the duration.
  if (execution_time_usec < 0LL) {
    LOG(DFATAL) << "Bad execution_time_usec: " << execution_time_usec;
    return 0LL;
  }
  if (end_time_usec - start_time_usec < execution_time_usec) {
    LOG(DFATAL) << "execution_time_usec lt window: "
                << execution_time_usec << " > "
                << start_time_usec << "-" << end_time_usec;
    return 0LL;
  }

  return execution_time_usec;
}

int64 GetOwnExecutionTimeUsec(
    const CallTree &tree,
    int64 start_time_usec,
    int64 end_time_usec) {
  if (start_time_usec < 0LL) {
    LOG(DFATAL) << "Bad start_time_usec: " << start_time_usec;
    return 0LL;
  }
  if (end_time_usec < start_time_usec) {
    LOG(DFATAL) << "end_time_usec lt start_time_usec: "
                << end_time_usec << " < " << start_time_usec;
    return 0LL;
  }

  // First compute the total execution time for this node.
  int64 execution_time_usec =
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
  if (execution_time_usec < 0LL) {
    LOG(DFATAL) << "Bad execution_time_usec: " << execution_time_usec;
    return 0LL;
  }
  if (end_time_usec - start_time_usec < execution_time_usec) {
    LOG(DFATAL) << "execution_time_usec lt window: "
                << execution_time_usec << " > "
                << start_time_usec << "-" << end_time_usec;
    return 0LL;
  }

  return execution_time_usec;
}

void PopulateFunctionInitCounts(
    const CallGraphProfileSnapshot &snapshot,
    CallGraphTimelineEventSet *events,
    int64 start_time_usec,
    int64 end_time_usec) {
  if (start_time_usec < 0LL) {
    LOG(DFATAL) << "Bad start_time_usec: " << start_time_usec;
    return;
  }
  if (end_time_usec < 0LL) {
    LOG(DFATAL) << "Bad end_time_usec: " << end_time_usec;
    return;
  }
  if (end_time_usec < start_time_usec) {
    LOG(DFATAL) << "end_time_usec lt start_time_usec: "
                << end_time_usec << " < " << start_time_usec;
    return;
  }

  const CallGraphProfileSnapshot::InitTimeMap *init_time_map =
      snapshot.init_time_map();
  for (CallGraphProfileSnapshot::InitTimeMap::const_iterator
           it = init_time_map->lower_bound(start_time_usec),
           end = init_time_map->lower_bound(end_time_usec);
       it != end;
       ++it) {
    const FunctionMetadata *metadata = it->second;
    const int64 function_instantiation_time_usec =
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
    int64 start_time_usec,
    int64 end_time_usec) {
  if (start_time_usec < 0LL) {
    LOG(DFATAL) << "Bad start_time_usec: " << start_time_usec;
    return;
  }
  if (end_time_usec < 0LL) {
    LOG(DFATAL) << "Bad end_time_usec: " << end_time_usec;
    return;
  }
  if (end_time_usec < start_time_usec) {
    LOG(DFATAL) << "end_time_usec lt start_time_usec: "
                << end_time_usec << " < " << start_time_usec;
    return;
  }
  if (start_time_usec % events->event_duration_usec() != 0) {
    LOG(DFATAL) << "start_time_usec: " << start_time_usec
                << " not a multiple of " << events->event_duration_usec();
    return;
  }
  if (end_time_usec != LONG_LONG_MAX) {
    if (end_time_usec % events->event_duration_usec() != 0) {
      LOG(DFATAL) << "end_time_usec: " << end_time_usec
                  << " not a multiple of " << events->event_duration_usec();
      return;
    }
  }
  if (events == NULL) {
    LOG(DFATAL) << "events is null.";
    return;
  }

  CallGraphTimelineVisitor visitor(
      new TimeRangeVisitFilter(start_time_usec, end_time_usec),
      *snapshot.metadata(),
      events,
      start_time_usec,
      end_time_usec);

  snapshot.call_graph()->Traverse(&visitor);
}

int64 GetMaxFullyConstructedCallGraphTimeUsec(const CallGraph &call_graph) {
  int64 max_time_usec = 0LL;
  const CallGraph::CallForest *forest = call_graph.call_forest();
  if (!forest->empty()) {
    max_time_usec = forest->back()->exit_time_usec();
  }
  return max_time_usec;
}

void FormatTime(int64 time_usec, std::string *target) {
  // Very simple format for now: if under 10 seconds, show
  // milliseconds. Otherwise, truncate to whole seconds.
  const long long msec = time_usec / 1000LL;

  // 20 characters is enough to fit any 64 bit number divided by 1000.
  const size_t kBufSize = 20;
  char tmp[kBufSize];
  if (msec < 10000LL) {
    snprintf(tmp, kBufSize, "%lld ms", msec);
  } else {
    snprintf(tmp, kBufSize, "%lld seconds", (msec / 1000LL));
  }

  target->append(tmp, kBufSize);
}

}  // namespace util

}  // namespace activity
