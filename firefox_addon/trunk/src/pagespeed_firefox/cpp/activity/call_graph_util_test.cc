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

#include <limits>
#include <vector>

#include "call_graph_util.h"

#include "base/scoped_ptr.h"
#include "call_graph.h"
#include "call_graph_metadata.h"
#include "call_graph_profile.h"
#include "call_graph_profile_snapshot.h"
#include "call_graph_timeline_event.h"
#include "call_graph_timeline_event_set.h"
#include "clock.h"
#include "profile.pb.h"
#include "test_stub_function_info.h"
#include "timer.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace {

TEST(CallGraphUtilRoundTest, RoundDownMultiple1) {
  for (int64 i = 0; i < 100; ++i) {
    ASSERT_EQ(
        i,
        activity::util::RoundDownToNearestWholeMultiple(i, 1LL)) << i;
  }
}

TEST(CallGraphUtilRoundTest, RoundUpMultiple1) {
  for (int64 i = 0; i < 100; ++i) {
    ASSERT_EQ(
        i,
        activity::util::RoundUpToNearestWholeMultiple(i, 1LL)) << i;
  }
}

TEST(CallGraphUtilRoundTest, RoundDownMultiple2) {
  for (int64 i = 0; i < 100; ++i) {
    ASSERT_EQ(
        i - (i % 2),
        activity::util::RoundDownToNearestWholeMultiple(i, 2LL)) << i;
  }
}

TEST(CallGraphUtilRoundTest, RoundUpMultiple2) {
  for (int64 i = 0; i < 100; ++i) {
    ASSERT_EQ(
        i + (i % 2),
        activity::util::RoundUpToNearestWholeMultiple(i, 2LL)) << i;
  }
}

TEST(CallGraphUtilRoundTest, RoundDownMax) {
  ASSERT_EQ(0LL, activity::util::RoundDownToNearestWholeMultiple(
                LONG_LONG_MAX - 1LL, LONG_LONG_MAX));
  ASSERT_EQ(0LL, activity::util::RoundDownToNearestWholeMultiple(
                1LL, LONG_LONG_MAX));
  ASSERT_EQ(LONG_LONG_MAX, activity::util::RoundDownToNearestWholeMultiple(
                LONG_LONG_MAX, LONG_LONG_MAX));
}

TEST(CallGraphUtilRoundTest, RoundUpMax) {
  ASSERT_EQ(LONG_LONG_MAX, activity::util::RoundUpToNearestWholeMultiple(
                LONG_LONG_MAX - 1LL, LONG_LONG_MAX));
  ASSERT_EQ(LONG_LONG_MAX, activity::util::RoundUpToNearestWholeMultiple(
                1LL, LONG_LONG_MAX));
  ASSERT_EQ(LONG_LONG_MAX, activity::util::RoundUpToNearestWholeMultiple(
                LONG_LONG_MAX, LONG_LONG_MAX));

  // Show that when rounding up would cause overflow, we round down to
  // the nearest whole multiple.
  const int64 largest_mutliple_of_8 = LONG_LONG_MAX / 8LL * 8LL;
  ASSERT_LT(largest_mutliple_of_8, LONG_LONG_MAX);
  ASSERT_EQ(largest_mutliple_of_8,
            activity::util::RoundUpToNearestWholeMultiple(
                LONG_LONG_MAX, 8LL));
}

class CallGraphUtilExecutionTimeTest : public testing::Test {
 protected:

  virtual void SetUp() {
    tree_.set_entry_time_usec(0LL);
    tree_.set_exit_time_usec(25LL);

    child1_ = tree_.add_children();
    child1_->set_entry_time_usec(3LL);
    child1_->set_exit_time_usec(8LL);

    child2_ = tree_.add_children();
    child2_->set_entry_time_usec(10LL);
    child2_->set_exit_time_usec(15LL);

    child3_ = tree_.add_children();
    child3_->set_entry_time_usec(15LL);
    child3_->set_exit_time_usec(25LL);
  }

  activity::CallTree tree_;
  activity::CallTree *child1_;
  activity::CallTree *child2_;
  activity::CallTree *child3_;
};

TEST_F(CallGraphUtilExecutionTimeTest, TotalExecutionTime) {
  const int64 start_time_usec = 0LL;
  const int64 end_time_usec = LONG_LONG_MAX;

  ASSERT_EQ(
      25LL, activity::util::GetTotalExecutionTimeUsec(
          tree_, start_time_usec, end_time_usec));

  ASSERT_EQ(
      5LL, activity::util::GetTotalExecutionTimeUsec(
          *child1_, start_time_usec, end_time_usec));

  ASSERT_EQ(
      5LL, activity::util::GetTotalExecutionTimeUsec(
          *child2_, start_time_usec, end_time_usec));

  ASSERT_EQ(
      10LL, activity::util::GetTotalExecutionTimeUsec(
          *child3_, start_time_usec, end_time_usec));
}

TEST_F(CallGraphUtilExecutionTimeTest,
       TotalExecutionTimeWindowOfZero) {
  const int64 start_time_usec = 0LL;
  const int64 end_time_usec = 0LL;

  ASSERT_EQ(
      0LL, activity::util::GetTotalExecutionTimeUsec(
          tree_, start_time_usec, end_time_usec));

  ASSERT_EQ(
      0LL, activity::util::GetTotalExecutionTimeUsec(
          *child1_, start_time_usec, end_time_usec));

  ASSERT_EQ(
      0LL, activity::util::GetTotalExecutionTimeUsec(
          *child2_, start_time_usec, end_time_usec));

  ASSERT_EQ(
      0LL, activity::util::GetTotalExecutionTimeUsec(
          *child3_, start_time_usec, end_time_usec));
}

TEST_F(CallGraphUtilExecutionTimeTest,
       TotalExecutionTimePartialWindow) {
  const int64 start_time_usec = 4LL;
  const int64 end_time_usec = 17LL;

  ASSERT_EQ(
      end_time_usec - start_time_usec,
      activity::util::GetTotalExecutionTimeUsec(
          tree_, start_time_usec, end_time_usec));

  ASSERT_EQ(
      4LL, activity::util::GetTotalExecutionTimeUsec(
          *child1_, start_time_usec, end_time_usec));

  ASSERT_EQ(
      5LL, activity::util::GetTotalExecutionTimeUsec(
          *child2_, start_time_usec, end_time_usec));

  ASSERT_EQ(
      2LL, activity::util::GetTotalExecutionTimeUsec(
          *child3_, start_time_usec, end_time_usec));
}

TEST_F(CallGraphUtilExecutionTimeTest, OwnExecutionTime) {
  const int64 start_time_usec = 0LL;
  const int64 end_time_usec = LONG_LONG_MAX;

  ASSERT_EQ(
      5LL, activity::util::GetOwnExecutionTimeUsec(
          tree_, start_time_usec, end_time_usec));

  ASSERT_EQ(
      5LL, activity::util::GetOwnExecutionTimeUsec(
          *child1_, start_time_usec, end_time_usec));

  ASSERT_EQ(
      5LL, activity::util::GetOwnExecutionTimeUsec(
          *child2_, start_time_usec, end_time_usec));

  ASSERT_EQ(
      10LL, activity::util::GetOwnExecutionTimeUsec(
          *child3_, start_time_usec, end_time_usec));
}

TEST_F(CallGraphUtilExecutionTimeTest, OwnExecutionTimeWindowOfZero) {
  const int64 start_time_usec = 0LL;
  const int64 end_time_usec = 0LL;

  ASSERT_EQ(
      0LL, activity::util::GetOwnExecutionTimeUsec(
          tree_, start_time_usec, end_time_usec));

  ASSERT_EQ(
      0LL, activity::util::GetOwnExecutionTimeUsec(
          *child1_, start_time_usec, end_time_usec));

  ASSERT_EQ(
      0LL, activity::util::GetOwnExecutionTimeUsec(
          *child2_, start_time_usec, end_time_usec));

  ASSERT_EQ(
      0LL, activity::util::GetOwnExecutionTimeUsec(
          *child3_, start_time_usec, end_time_usec));
}

TEST_F(CallGraphUtilExecutionTimeTest, OwnExecutionTimePartialWindow) {
  const int64 start_time_usec = 4LL;
  const int64 end_time_usec = 17LL;

  ASSERT_EQ(
      2LL, activity::util::GetOwnExecutionTimeUsec(
          tree_, start_time_usec, end_time_usec));

  ASSERT_EQ(
      4LL, activity::util::GetOwnExecutionTimeUsec(
          *child1_, start_time_usec, end_time_usec));

  ASSERT_EQ(
      5LL, activity::util::GetOwnExecutionTimeUsec(
          *child2_, start_time_usec, end_time_usec));

  ASSERT_EQ(
      2LL, activity::util::GetOwnExecutionTimeUsec(
          *child3_, start_time_usec, end_time_usec));
}

const int32 kFunctionTag1 = 1;
const int32 kFunctionTag2 = 2;
const int32 kFunctionTag3 = 3;
const int32 kFunctionTag4 = 4;
const int32 kFunctionTag5 = 5;

const char *kFileName1 = "foo.js";
const char *kFileName2 = "bar.js";

const char *kFunctionName1 = "func1";
const char *kFunctionName2 = "func2";
const char *kFunctionName3 = "func3";
const char *kFunctionName4 = "func4";
const char *kFunctionName5 = "func5";

const char *kFunctionSource1 = "function func1() { foo(); }";
const char *kFunctionSource2 = "function func2() { foo(); }";
const char *kFunctionSource3 = "function func3() { foo(); }";
const char *kFunctionSource4 = "function func4() { foo(); }";
const char *kFunctionSource5 = "function func5() { foo(); }";

const int64 kFunctionInit1 = 0LL;
const int64 kFunctionInit2 = 5LL;
const int64 kFunctionInit3 = 15LL;
const int64 kFunctionInit4 = 20LL;
const int64 kFunctionInit5 = 20LL;

const int64 kEventDurationUsec = 10LL;

class CallGraphUtilEventSetTestBase : public testing::Test {
 protected:

  virtual void SetUp() {
    clock_.reset(new activity_testing::MockClock());
    profile_.reset(
        new activity::CallGraphProfile(clock_.get()));
    profile_->Start();
    function_info_1_.reset(
        new activity_testing::TestStubFunctionInfo(
            kFunctionTag1, kFileName1, kFunctionName1, kFunctionSource1));
    function_info_2_.reset(
        new activity_testing::TestStubFunctionInfo(
            kFunctionTag2, kFileName1, kFunctionName2, kFunctionSource2));
    function_info_3_.reset(
        new activity_testing::TestStubFunctionInfo(
            kFunctionTag3, kFileName2, kFunctionName3, kFunctionSource3));
    function_info_4_.reset(
        new activity_testing::TestStubFunctionInfo(
            kFunctionTag4, kFileName2, kFunctionName4, kFunctionSource4));
    function_info_5_.reset(
        new activity_testing::TestStubFunctionInfo(
            kFunctionTag5, kFileName1, kFunctionName5, kFunctionSource5));
    event_set_.reset(
        new activity::CallGraphTimelineEventSet(kEventDurationUsec));
  }

  void AppendTrace() {
    /* Append the following simple call tree:
          1
         / \
        2   5
       / \
      3   4
      Profile start time: 0usec

      Node 1 entry time:  2usec
      Node 1 exit time:   31usec
      Node 1 exec time:   2-7, 24-25, 28-31

      Node 2 entry time:  7usec
      Node 2 exit time:   24usec
      Node 2 exec time:   7-17, 19-22, 23-24

      Node 3 entry time:  17usec
      Node 3 exit time:   19usec
      Node 3 exec time:   17-19

      Node 4 entry time:  22usec
      Node 4 exit time:   23usec
      Node 4 exec time:   22-23

      Node 5 entry time:  25usec
      Node 5 exit time:   28usec
      Node 5 exec time:   25-28
    */

    clock_->current_time_usec_ = kFunctionInit1;
    profile_->OnFunctionInstantiated(function_info_1_.get());

    clock_->current_time_usec_ = kFunctionInit1 + 2LL;
    profile_->OnFunctionEntry();  // 1

    clock_->current_time_usec_ = kFunctionInit2;
    profile_->OnFunctionInstantiated(function_info_2_.get());

    clock_->current_time_usec_ = kFunctionInit2 + 2LL;
    profile_->OnFunctionEntry();  // 2

    clock_->current_time_usec_ = kFunctionInit3;
    profile_->OnFunctionInstantiated(function_info_3_.get());

    clock_->current_time_usec_ = kFunctionInit3 + 2LL;
    profile_->OnFunctionEntry();  // 3

    clock_->current_time_usec_ = kFunctionInit3 + 4LL;
    profile_->OnFunctionExit(function_info_3_.get());

    clock_->current_time_usec_ = kFunctionInit4;
    profile_->OnFunctionInstantiated(function_info_4_.get());

    // same as kFunctionInit4
    clock_->current_time_usec_ = kFunctionInit5;
    profile_->OnFunctionInstantiated(function_info_5_.get());

    clock_->current_time_usec_ = kFunctionInit4 + 2LL;
    profile_->OnFunctionEntry();  // 4

    profile_->OnFunctionExit(function_info_4_.get());

    clock_->current_time_usec_ = kFunctionInit4 + 4LL;
    profile_->OnFunctionExit(function_info_2_.get());

    clock_->current_time_usec_ = kFunctionInit5 + 5LL;
    profile_->OnFunctionEntry();  // 5

    clock_->current_time_usec_ = kFunctionInit5 + 8LL;
    profile_->OnFunctionExit(function_info_5_.get());

    clock_->current_time_usec_ = kFunctionInit5 + 11LL;
    profile_->OnFunctionExit(function_info_1_.get());
  }

  void DoPopulateEventVector(
      std::vector<const activity::CallGraphTimelineEvent*> *event_vector) {
    const activity::CallGraphTimelineEventSet::EventMap *event_map =
        event_set_->event_map();
    for (activity::CallGraphTimelineEventSet::EventMap::const_iterator iter =
             event_map->begin(), end = event_map->end();
         iter != end;
         ++iter) {
      event_vector->push_back(iter->second);
    }
  }

  void AssertMatchingEvent(
      const activity::CallGraphTimelineEvent *event,
      int64 expected_start_time_usec,
      const char *expected_file_name,
      activity::CallGraphTimelineEvent::Type expected_type,
      int64 expected_intensity) {
    ASSERT_EQ(expected_start_time_usec, event->start_time_usec);
    ASSERT_STREQ(expected_file_name, event->identifier);
    ASSERT_EQ(expected_intensity, event->intensity);
    ASSERT_EQ(expected_type, event->type);
  }

  scoped_ptr<activity_testing::MockClock> clock_;
  scoped_ptr<activity::CallGraphProfile> profile_;
  scoped_ptr<activity::FunctionInfoInterface> function_info_1_;
  scoped_ptr<activity::FunctionInfoInterface> function_info_2_;
  scoped_ptr<activity::FunctionInfoInterface> function_info_3_;
  scoped_ptr<activity::FunctionInfoInterface> function_info_4_;
  scoped_ptr<activity::FunctionInfoInterface> function_info_5_;
  scoped_ptr<activity::CallGraphTimelineEventSet> event_set_;
  std::vector<const activity::CallGraphTimelineEvent*> event_vector_;
};

class CallGraphUtilInitCountTest
    : public CallGraphUtilEventSetTestBase {
 protected:
  void DoPopulateFunctionInitCounts(
      int64 start_time_usec, int64_t end_time_usec) {
    scoped_ptr<activity::CallGraphProfileSnapshot> snapshot(
        profile_->CreateSnapshot());
    snapshot->Init(start_time_usec, end_time_usec);
    activity::util::PopulateFunctionInitCounts(
        *snapshot.get(),
        event_set_.get(),
        start_time_usec,
        end_time_usec);
    DoPopulateEventVector(&event_vector_);
  }

  // Because there are two events with the same init time, their sort
  // order is not deterministic. Thus, we assert that the two events
  // have the expected contents, without expecting them to appear in a
  // specific order.
  void AssertEventsAt20Usec(
      const activity::CallGraphTimelineEvent *event1,
      const activity::CallGraphTimelineEvent *event2) {
    ASSERT_EQ(20LL, event1->start_time_usec);
    ASSERT_EQ(20LL, event2->start_time_usec);
    ASSERT_EQ(activity::CallGraphTimelineEvent::JS_PARSE, event1->type);
    ASSERT_EQ(1, event1->intensity);
    ASSERT_EQ(activity::CallGraphTimelineEvent::JS_PARSE, event2->type);
    ASSERT_EQ(1, event2->intensity);
    ASSERT_STRNE(event1->identifier, event2->identifier);

    // Verify that one event has kFileName1, and the other has
    // kFileName2.
    if (event1->identifier == kFileName1) {
      ASSERT_STREQ(event2->identifier, kFileName2);
    } else {
      ASSERT_STREQ(event1->identifier, kFileName2);
      ASSERT_STREQ(event2->identifier, kFileName1);
    }
  }
};

TEST_F(CallGraphUtilInitCountTest, EmptyMetadata) {
  DoPopulateFunctionInitCounts(0LL, LONG_LONG_MAX);
  ASSERT_EQ(0, event_vector_.size());
}

TEST_F(CallGraphUtilInitCountTest, WindowOfZero) {
  AppendTrace();

  DoPopulateFunctionInitCounts(0LL, 0LL);
  ASSERT_EQ(0, event_vector_.size());

  DoPopulateFunctionInitCounts(LONG_LONG_MAX, LONG_LONG_MAX);
  ASSERT_EQ(0, event_vector_.size());
}

TEST_F(CallGraphUtilInitCountTest, BasicPopulate) {
  AppendTrace();
  DoPopulateFunctionInitCounts(0LL, LONG_LONG_MAX);

  ASSERT_EQ(4, event_vector_.size());
  AssertMatchingEvent(
      event_vector_[0],
      0LL,
      kFileName1,
      activity::CallGraphTimelineEvent::JS_PARSE,
      2);
  AssertMatchingEvent(
      event_vector_[1],
      10LL,
      kFileName2,
      activity::CallGraphTimelineEvent::JS_PARSE,
      1);
  AssertEventsAt20Usec(event_vector_[2], event_vector_[3]);
}

TEST_F(CallGraphUtilInitCountTest, LimitedWindow1) {
  AppendTrace();
  DoPopulateFunctionInitCounts(1LL, 5LL);
  ASSERT_EQ(0, event_vector_.size());
}

TEST_F(CallGraphUtilInitCountTest, LimitedWindow2) {
  AppendTrace();
  DoPopulateFunctionInitCounts(0LL, 5LL);

  ASSERT_EQ(1, event_vector_.size());
  AssertMatchingEvent(
      event_vector_[0],
      0LL,
      kFileName1,
      activity::CallGraphTimelineEvent::JS_PARSE,
      1);
}

TEST_F(CallGraphUtilInitCountTest, LimitedWindow3) {
  AppendTrace();
  DoPopulateFunctionInitCounts(1LL, 6LL);

  ASSERT_EQ(1, event_vector_.size());
  AssertMatchingEvent(
      event_vector_[0],
      0LL,
      kFileName1,
      activity::CallGraphTimelineEvent::JS_PARSE,
      1);
}

TEST_F(CallGraphUtilInitCountTest, LimitedWindow4) {
  AppendTrace();
  DoPopulateFunctionInitCounts(0LL, 6LL);

  ASSERT_EQ(1, event_vector_.size());
  AssertMatchingEvent(
      event_vector_[0],
      0LL,
      kFileName1,
      activity::CallGraphTimelineEvent::JS_PARSE,
      2);
}

TEST_F(CallGraphUtilInitCountTest, LimitedWindow5) {
  AppendTrace();
  DoPopulateFunctionInitCounts(5LL, 20LL);

  ASSERT_EQ(2, event_vector_.size());
  AssertMatchingEvent(
      event_vector_[0],
      0LL,
      kFileName1,
      activity::CallGraphTimelineEvent::JS_PARSE,
      1);
  AssertMatchingEvent(
      event_vector_[1],
      10LL,
      kFileName2,
      activity::CallGraphTimelineEvent::JS_PARSE,
      1);
}

TEST_F(CallGraphUtilInitCountTest, LimitedWindow6) {
  AppendTrace();
  DoPopulateFunctionInitCounts(5LL, 21LL);

  ASSERT_EQ(4, event_vector_.size());
  AssertMatchingEvent(
      event_vector_[0],
      0LL,
      kFileName1,
      activity::CallGraphTimelineEvent::JS_PARSE,
      1);
  AssertMatchingEvent(
      event_vector_[1],
      10LL,
      kFileName2,
      activity::CallGraphTimelineEvent::JS_PARSE,
      1);
  AssertEventsAt20Usec(event_vector_[2], event_vector_[3]);
}

TEST_F(CallGraphUtilInitCountTest, LimitedWindow7) {
  AppendTrace();
  DoPopulateFunctionInitCounts(20LL, 21LL);

  ASSERT_EQ(2, event_vector_.size());
  AssertEventsAt20Usec(event_vector_[0], event_vector_[1]);
}

class CallGraphUtilPopulateExecTimeTest
    : public CallGraphUtilEventSetTestBase {
 protected:
  void DoPopulateExecutionTimes(
      int64 start_time_usec, int64 end_time_usec) {
    scoped_ptr<activity::CallGraphProfileSnapshot> snapshot(
        profile_->CreateSnapshot());
    snapshot->Init(start_time_usec, end_time_usec);
    activity::util::PopulateExecutionTimes(
        *snapshot.get(),
        event_set_.get(),
        start_time_usec,
        end_time_usec);
    DoPopulateEventVector(&event_vector_);
  }
};

TEST_F(CallGraphUtilPopulateExecTimeTest, EmptyCallGraph) {
  DoPopulateExecutionTimes(0LL, LONG_LONG_MAX);
  ASSERT_EQ(0, event_vector_.size());
}

TEST_F(CallGraphUtilPopulateExecTimeTest, WindowOfZero) {
  AppendTrace();

  DoPopulateExecutionTimes(0LL, 0LL);
  ASSERT_EQ(0, event_vector_.size());

  DoPopulateExecutionTimes(
      activity::util::RoundDownToNearestWholeMultiple(
          LONG_LONG_MAX, event_set_->event_duration_usec()), LONG_LONG_MAX);
  ASSERT_EQ(0, event_vector_.size());
}

TEST_F(CallGraphUtilPopulateExecTimeTest, BasicCallGraph) {
  AppendTrace();

  DoPopulateExecutionTimes(0LL, LONG_LONG_MAX);
  ASSERT_EQ(6, event_vector_.size());

  AssertMatchingEvent(
      event_vector_[0],
      0LL,
      kFileName1,
      activity::CallGraphTimelineEvent::JS_EXECUTE,
      8);
  AssertMatchingEvent(
      event_vector_[1],
      10LL,
      kFileName2,
      activity::CallGraphTimelineEvent::JS_EXECUTE,
      2);
  AssertMatchingEvent(
      event_vector_[2],
      10LL,
      kFileName1,
      activity::CallGraphTimelineEvent::JS_EXECUTE,
      8);
  AssertMatchingEvent(
      event_vector_[3],
      20LL,
      kFileName2,
      activity::CallGraphTimelineEvent::JS_EXECUTE,
      1);
  AssertMatchingEvent(
      event_vector_[4],
      20LL,
      kFileName1,
      activity::CallGraphTimelineEvent::JS_EXECUTE,
      9);
  AssertMatchingEvent(
      event_vector_[5],
      30LL,
      kFileName1,
      activity::CallGraphTimelineEvent::JS_EXECUTE,
      1);
}

TEST_F(CallGraphUtilPopulateExecTimeTest, LimitedWindow1) {
  AppendTrace();
  DoPopulateExecutionTimes(0LL, 10LL);
  ASSERT_EQ(1, event_vector_.size());

  AssertMatchingEvent(
      event_vector_[0],
      0LL,
      kFileName1,
      activity::CallGraphTimelineEvent::JS_EXECUTE,
      8);
}

TEST_F(CallGraphUtilPopulateExecTimeTest, LimitedWindow2) {
  AppendTrace();
  DoPopulateExecutionTimes(10LL, 20LL);

  ASSERT_EQ(2, event_vector_.size());

  AssertMatchingEvent(
      event_vector_[0],
      10LL,
      kFileName2,
      activity::CallGraphTimelineEvent::JS_EXECUTE,
      2);
  AssertMatchingEvent(
      event_vector_[1],
      10LL,
      kFileName1,
      activity::CallGraphTimelineEvent::JS_EXECUTE,
      8);
}

TEST_F(CallGraphUtilPopulateExecTimeTest, LimitedWindow3) {
  AppendTrace();
  DoPopulateExecutionTimes(30LL, LONG_LONG_MAX);

  ASSERT_EQ(1, event_vector_.size());
  AssertMatchingEvent(
      event_vector_[0],
      30LL,
      kFileName1,
      activity::CallGraphTimelineEvent::JS_EXECUTE,
      1);
}

class CallGraphUtilMaxCallGraphTimeTest : public testing::Test {
 protected:
  virtual void SetUp() {
    profile_.reset(new activity::Profile);
    clock_.reset(new activity_testing::MockClock());
    timer_.reset(
        new activity::Timer(clock_.get(), clock_->GetCurrentTimeUsec()));
    graph_.reset(new activity::CallGraph(profile_.get(), timer_.get()));
  }

  scoped_ptr<activity::Profile> profile_;
  scoped_ptr<activity_testing::MockClock> clock_;
  scoped_ptr<activity::Timer> timer_;
  scoped_ptr<activity::CallGraph> graph_;
};

TEST_F(CallGraphUtilMaxCallGraphTimeTest, EmptyGraph) {
  ASSERT_EQ(
      0LL,
      activity::util::GetMaxFullyConstructedCallGraphTimeUsec(*graph_.get()));
}

TEST_F(CallGraphUtilMaxCallGraphTimeTest, FullGraph) {
  graph_->OnFunctionEntry();
  graph_->OnFunctionExit(0);
  graph_->OnFunctionEntry();
  graph_->OnFunctionExit(0);
  const int32 time_after_last_tree =
      graph_->call_forest()->back()->exit_time_usec();
  ASSERT_EQ(
      time_after_last_tree,
      activity::util::GetMaxFullyConstructedCallGraphTimeUsec(*graph_.get()));
}

TEST_F(CallGraphUtilMaxCallGraphTimeTest, PartialGraph) {
  graph_->OnFunctionEntry();
  graph_->OnFunctionExit(0);
  const int32 time_after_first_tree =
      graph_->call_forest()->back()->exit_time_usec();
  graph_->OnFunctionEntry();
  ASSERT_EQ(
      time_after_first_tree,
      activity::util::GetMaxFullyConstructedCallGraphTimeUsec(*graph_.get()));
}

}  // namespace
