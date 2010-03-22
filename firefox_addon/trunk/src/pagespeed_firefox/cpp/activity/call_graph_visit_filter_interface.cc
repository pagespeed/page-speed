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
// CallGraphVisitFilterInterface implementation

#include "call_graph_visit_filter_interface.h"

#include "base/logging.h"
#include "profile.pb.h"

namespace activity {

CallGraphVisitFilterInterface::CallGraphVisitFilterInterface() {}

CallGraphVisitFilterInterface::~CallGraphVisitFilterInterface() {}

AlwaysVisitFilter::AlwaysVisitFilter() {}

AlwaysVisitFilter::~AlwaysVisitFilter() {}

bool AlwaysVisitFilter::ShouldTraverse(
    const CallTree &tree, const std::vector<const CallTree*>& parent_stack) {
  return true;
}

CompositeVisitFilter::CompositeVisitFilter(
    CallGraphVisitFilterInterface *first,
    CallGraphVisitFilterInterface *second)
    : first_(first),
      second_(second) {
  if (first == NULL || second == NULL) {
    LOG(DFATAL) << "Bad inputs: " << first << "," << second;
  }
}

CompositeVisitFilter::~CompositeVisitFilter() {
}

bool CompositeVisitFilter::ShouldTraverse(
    const CallTree &tree, const std::vector<const CallTree*>& parent_stack) {
  return first_->ShouldTraverse(tree, parent_stack) &&
      second_->ShouldTraverse(tree, parent_stack);
}

TimeRangeVisitFilter::TimeRangeVisitFilter(
    int64 start_time_usec,
    int64 end_time_usec)
    : start_time_usec_(start_time_usec),
      end_time_usec_(end_time_usec) {
  if (end_time_usec < start_time_usec) {
    LOG(DFATAL) << "end_time_usec lt start_time_usec: "
                << end_time_usec_ << " < " << start_time_usec_;
  }
}

TimeRangeVisitFilter::~TimeRangeVisitFilter() {}

bool TimeRangeVisitFilter::ShouldTraverse(
    const CallTree &tree, const std::vector<const CallTree*>& parent_stack) {
  return tree.entry_time_usec() < end_time_usec_ &&
      tree.exit_time_usec() >= start_time_usec_;
}

}  // namespace activity
