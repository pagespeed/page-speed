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

#include "base/scoped_ptr.h"
#include "call_graph.h"
#include "call_graph_profile_snapshot.h"
#include "call_graph_timeline_event_set.h"
#include "call_graph_timeline_test_base.h"
#include "call_graph_util.h"

#include "testing/gtest/include/gtest/gtest.h"

#ifdef _WINDOWS

// Provide LONG_LONG_MAX, which is available under a different name on
// windows.
#define LONG_LONG_MAX _I64_MAX

#endif

namespace {

int64 GetStopTimeUsec(const activity::CallGraphProfile &profile) {
  return profile.profile()->start_time_usec() +
      profile.profile()->duration_usec();
}

class CallGraphTimelineVisitorTest
    : public activity_testing::CallGraphTimelineTestBase {
 protected:
  void DoTraverse(int64 start_time_usec, int64 end_time_usec) {
    scoped_ptr<activity::CallGraphProfileSnapshot> snapshot(
        profile_->CreateSnapshot());
    snapshot->Init(start_time_usec, end_time_usec);
    activity::CallGraphTimelineVisitor visitor(
        NULL,
        *snapshot->metadata(),
        event_set_.get(),
        start_time_usec,
        end_time_usec);
    snapshot->call_graph()->Traverse(&visitor);
  }
};

TEST_F(CallGraphTimelineVisitorTest, BasicVisit) {
  const int64 start_time_usec = 0LL;
  const int64 end_time_usec = LONG_LONG_MAX;
  const int64 resolution_usec = 1LL;

  InitializeEventSet(resolution_usec);

  // Traverse the call graph, which will populate the event set.
  DoTraverse(start_time_usec, end_time_usec);

  AppendEventsToVector();

  AssertBasicTest(resolution_usec);
}

TEST_F(CallGraphTimelineVisitorTest, NoVisit) {
  for (int64 start_time_usec = 0LL;
       start_time_usec <= GetStopTimeUsec(*profile_.get());
       ++start_time_usec) {
    // Specify a time window of zero. We expect the resulting event
    // vector to be empty.
    const int64 end_time_usec = start_time_usec;
    const int64 resolution_usec = 1LL;

    InitializeEventSet(resolution_usec);

    // Traverse the call graph, which will populate the event set.
    DoTraverse(start_time_usec, end_time_usec);

    AppendEventsToVector();

    ASSERT_TRUE(event_vector_.empty());
  }
}

TEST_F(CallGraphTimelineVisitorTest, NonOverlappingTimeWindowVisit) {
  StopProfiling();

  // Specify a time window beyond the end of the profile. We expect
  // the resulting event vector to be empty.
  const int64 start_time_usec = GetStopTimeUsec(*profile_.get());
  const int64 end_time_usec = LONG_LONG_MAX;
  const int64 resolution_usec = 1LL;

  InitializeEventSet(resolution_usec);

  // Traverse the call graph, which will populate the event set.
  DoTraverse(start_time_usec, end_time_usec);

  AppendEventsToVector();

  ASSERT_TRUE(event_vector_.empty());
}

TEST_F(CallGraphTimelineVisitorTest, BrokenUpVisit) {
  StopProfiling();

  const int64 start_time_usec = 0LL;
  const int64 end_time_usec = GetStopTimeUsec(*profile_.get());
  const int64 resolution_usec = 1LL;

  InitializeEventSet(resolution_usec);

  for (int64 bucket_start_usec = start_time_usec;
       bucket_start_usec < end_time_usec;
       bucket_start_usec += resolution_usec) {
    // Traverse the call graph one resolution at a time, which will
    // populate the event set.
    DoTraverse(
        bucket_start_usec,
        bucket_start_usec + resolution_usec);
  }

  AppendEventsToVector();

  AssertBasicTest(resolution_usec);
}

TEST_F(CallGraphTimelineVisitorTest, BrokenUpVisitStepByTwo) {
  StopProfiling();

  const int64 start_time_usec = 0LL;
  const int64 resolution_usec = 1LL;
  const int64 step_size = resolution_usec * 2LL;
  const int64 end_time_usec =
      activity::util::RoundUpToNearestWholeMultiple(
          GetStopTimeUsec(*profile_.get()), step_size);

  InitializeEventSet(resolution_usec);

  for (int64 bucket_start_usec = start_time_usec;
       bucket_start_usec < end_time_usec;
       bucket_start_usec += step_size) {
    // Traverse the call graph two resolutions at a time, which will
    // populate the event set.
    DoTraverse(
        bucket_start_usec,
        bucket_start_usec + step_size);
  }

  AppendEventsToVector();

  AssertBasicTest(resolution_usec);
}

TEST_F(CallGraphTimelineVisitorTest, PartialVisit) {
  ASSERT_GT(profile_->profile()->call_tree_size(), 0LL);
  const int64 start_time_usec =
      profile_->profile()->call_tree(0LL).entry_time_usec() + 2LL;
  const int64 end_time_usec = LONG_LONG_MAX;
  const int64 resolution_usec = 1LL;

  InitializeEventSet(resolution_usec);

  // Traverse the call graph, which will populate the event set.
  DoTraverse(start_time_usec, end_time_usec);

  AppendEventsToVector();

  AssertPartialTest(resolution_usec);
}

TEST_F(CallGraphTimelineVisitorTest, LowResolutionVisit) {
  const int64 start_time_usec = 0LL;
  const int64 end_time_usec = LONG_LONG_MAX;
  const int64 resolution_usec = 3LL;

  InitializeEventSet(resolution_usec);

  // Traverse the call graph, which will populate the event set.
  DoTraverse(start_time_usec, end_time_usec);

  AppendEventsToVector();

  AssertLowResolutionTest(start_time_usec, resolution_usec);
}

TEST_F(CallGraphTimelineVisitorTest, LowResolutionBrokenUpVisit) {
  StopProfiling();

  const int64 start_time_usec = 0LL;
  const int64 resolution_usec = 3LL;
  const int64 end_time_usec =
      activity::util::RoundUpToNearestWholeMultiple(
          GetStopTimeUsec(*profile_.get()), resolution_usec);

  InitializeEventSet(resolution_usec);

  for (int64 bucket_start_usec = start_time_usec;
       bucket_start_usec < end_time_usec;
       bucket_start_usec += resolution_usec) {
    // Traverse the call graph, which will populate the event set.
    DoTraverse(
        bucket_start_usec,
        bucket_start_usec + resolution_usec);
  }

  AppendEventsToVector();

  AssertLowResolutionTest(start_time_usec, resolution_usec);
}

}  // namespace
