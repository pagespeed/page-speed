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

#include "call_graph_timeline_visitor.h"

#include "call_graph_metadata.h"
#include "call_graph_profile.h"
#include "call_graph_timeline_event.h"
#include "call_graph_timeline_event_set.h"
#include "call_graph_util.h"
#include "profile.pb.h"
#include "check.h"

namespace activity {

CallGraphTimelineVisitor::CallGraphTimelineVisitor(
    CallGraphVisitFilterInterface *filter,
    const CallGraphMetadata &metadata,
    CallGraphTimelineEventSet *event_set,
    int64_t start_time_usec,
    int64_t end_time_usec)
    : CallGraphVisitorInterface(filter),
      metadata_(metadata),
      event_set_(event_set),
      start_time_usec_(start_time_usec),
      end_time_usec_(end_time_usec) {
  GCHECK_GE(start_time_usec_, 0LL);
  GCHECK_GE(end_time_usec_, 0LL);
  GCHECK_GE(end_time_usec_, start_time_usec_);
}

CallGraphTimelineVisitor::~CallGraphTimelineVisitor() {}

void CallGraphTimelineVisitor::OnEntry(
    const std::vector<const CallTree*>& stack) {
  const CallTree &tree = *stack.back();

  // Look up the URL associated with the function.
  const int32_t function_tag = tree.function_tag();
  const CallGraphMetadata::MetadataMap::const_iterator it =
      metadata_.map()->find(function_tag);
  GCHECK(it != metadata_.map()->end());
  const FunctionMetadata &data = *it->second;
  const char *identifier = data.file_name().c_str();

  if (CallGraphProfile::ShouldIncludeInProfile(identifier)) {
    RecordTimelineEvents(tree, identifier);
  }
}

void CallGraphTimelineVisitor::RecordTimelineEvents(
    const CallTree &tree,
    const char *identifier) {
  int64_t function_start_time_usec = -1LL;
  int64_t function_end_time_usec = -1LL;
  GetRoundedStartTimeAndEndTime(
      tree, &function_start_time_usec, &function_end_time_usec);

  if (function_start_time_usec == function_end_time_usec) {
    return;
  }

  // Populate the events associated with each bucket.
  for (int64_t bucket_start_time_usec = function_start_time_usec;
       bucket_start_time_usec < function_end_time_usec;
       bucket_start_time_usec += event_set_->event_duration_usec()) {
    const int64_t own_execution_time_usec =
        util::GetOwnExecutionTimeUsec(
            tree,
            bucket_start_time_usec,
            bucket_start_time_usec + event_set_->event_duration_usec());

    // We intentionally create an event even if the intensity is zero,
    // because we want to show that this function's file was on the
    // call stack in the UI. This also makes sure that we render very
    // short-lived events (under 1usec) in the UI, which happens due
    // to the Clock not having true usec resolution on some platforms.
    CallGraphTimelineEvent *event = event_set_->GetOrCreateEvent(
        identifier,
        CallGraphTimelineEvent::JS_EXECUTE,
        bucket_start_time_usec);
    event->intensity += own_execution_time_usec;
  }
}

void CallGraphTimelineVisitor::GetRoundedStartTimeAndEndTime(
    const CallTree &tree,
    int64_t *out_rounded_start_time_usec,
    int64_t *out_rounded_end_time_usec) const {
  GCHECK(out_rounded_start_time_usec != NULL);
  GCHECK(out_rounded_end_time_usec != NULL);

  const int64_t function_entry_time_usec = tree.entry_time_usec();
  int64_t function_exit_time_usec = tree.exit_time_usec();
  if (function_exit_time_usec == function_entry_time_usec) {
    // Special case: for 0usec function executions, make sure that the
    // start/end time spans one resolution. This guarantees that we
    // will generate an event for these events, so we will show them
    // in the UI.
    function_exit_time_usec++;
  }

  // Round the start time down to the nearest whole multiple.
  int64_t rounded_start_time_usec = util::RoundDownToNearestWholeMultiple(
      function_entry_time_usec, event_set_->event_duration_usec());

  // Round the end time up to the nearest whole multiple.
  int64_t rounded_end_time_usec = util::RoundUpToNearestWholeMultiple(
      function_exit_time_usec, event_set_->event_duration_usec());

  // Clamp to the requested start time.
  if (rounded_start_time_usec < start_time_usec_) {
    rounded_start_time_usec = start_time_usec_;
  }

  // Clamp to the requested end time.
  if (rounded_end_time_usec > end_time_usec_) {
    rounded_end_time_usec = end_time_usec_;
  }

  // Make sure end time does not come before start time.
  if (rounded_end_time_usec < rounded_start_time_usec) {
    rounded_end_time_usec = rounded_start_time_usec;
  }

  GCHECK_GE(rounded_start_time_usec, 0LL);
  GCHECK_GE(rounded_end_time_usec, 0LL);
  GCHECK_GE(rounded_end_time_usec, rounded_start_time_usec);
  GCHECK_EQ(0LL, rounded_start_time_usec % event_set_->event_duration_usec());
  GCHECK_EQ(0LL, rounded_end_time_usec % event_set_->event_duration_usec());

  *out_rounded_start_time_usec = rounded_start_time_usec;
  *out_rounded_end_time_usec = rounded_end_time_usec;
}

void CallGraphTimelineVisitor::OnExit(
    const std::vector<const CallTree*>& stack) {
}

}  // namespace activity
