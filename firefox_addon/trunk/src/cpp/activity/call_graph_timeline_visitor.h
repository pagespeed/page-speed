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
// CallGraphTimelineVisitor walks the call graph to build a timeline
// of CallGraphTimelineEvents.

#ifndef CALL_GRAPH_TIMELINE_VISITOR_H_
#define CALL_GRAPH_TIMELINE_VISITOR_H_

#include <stdint.h>

#include <vector>

#include "base/macros.h"
#include "call_graph_visitor_interface.h"

namespace activity {

class CallGraphMetadata;
class CallGraphTimelineEventSet;

class CallGraphTimelineVisitor : public CallGraphVisitorInterface {
 public:
  /**
   * @param filter the visit filter to be used, or NULL if no visit
   *     filter should be used.
   * @param metadata the call graph's metadata.
   * @param event_set the event set to populate.
   * @param start_time_usec the start time for this profiling run.
   * @param end_time_usec the end time for this profiling run.
   */
  CallGraphTimelineVisitor(
      CallGraphVisitFilterInterface *filter,
      const CallGraphMetadata &metadata,
      CallGraphTimelineEventSet *event_set,
      int64_t start_time_usec,
      int64_t end_time_usec);

  virtual ~CallGraphTimelineVisitor();

  virtual void OnEntry(const std::vector<const CallTree*>& stack);
  virtual void OnExit(const std::vector<const CallTree*>& stack);

 private:
  /**
   * Record the timeline events associated with the given CallTree.
   */
  void RecordTimelineEvents(const CallTree &tree, const char *identifier);

  /**
   * Helper that rounds the start time and end time of the given call
   * tree, based on the resolution, start time, and end time for this
   * CallGraphTimelineVisitor.
   */
  void GetRoundedStartTimeAndEndTime(
      const CallTree &tree,
      int64_t *rounded_start_time_usec,
      int64_t *rounded_end_time_usec) const;

  const CallGraphMetadata &metadata_;
  CallGraphTimelineEventSet *const event_set_;
  const int64_t start_time_usec_;
  const int64_t end_time_usec_;

  DISALLOW_COPY_AND_ASSIGN(CallGraphTimelineVisitor);
};

}  // namespace activity

#endif  // CALL_GRAPH_TIMELINE_VISITOR_H_
