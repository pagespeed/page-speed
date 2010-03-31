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
// Base test class used to test CallGraphTimeline and
// CallGraphTimelineVisitor.

#ifndef CALL_GRAPH_TIMELINE_TEST_BASE_H_
#define CALL_GRAPH_TIMELINE_TEST_BASE_H_

#include <vector>

#include "base/scoped_ptr.h"
#include "call_graph_profile.h"
#include "call_graph_timeline_event.h"
#include "call_graph_timeline_event_set.h"
#include "clock.h"
#include "profile.pb.h"
#include "test_stub_function_info.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace activity_testing {

const char* kTestUrl1 = "http://foo.com/index.html";
const char* kTestUrl2 = "http://bar.com/index.html";

const char* kTestName1 = "f1";
const char* kTestName2 = "f2";
const char* kTestName3 = "f3";

const char* kTestSource1 = "function f1() {}";
const char* kTestSource2 = "function f2() {}";
const char* kTestSource3 = "function f3() {}";

class CallGraphTimelineTestBase : public testing::Test {
 protected:
  virtual void SetUp() {
    clock_.reset(new activity_testing::MockClock());
    profile_.reset(new activity::CallGraphProfile(clock_.get()));
    profile_->Start();
    function_info_1_.reset(new activity_testing::TestStubFunctionInfo(
            1, kTestUrl1, kTestName1, kTestSource1));
    function_info_2_.reset(new activity_testing::TestStubFunctionInfo(
            2, kTestUrl2, kTestName2, kTestSource2));
    function_info_3_.reset(new activity_testing::TestStubFunctionInfo(
            3, kTestUrl1, kTestName3, kTestSource3));
    event_set_.reset(NULL);
    event_vector_.clear();

    AppendTrace();
  }

  virtual void TearDown() {
    StopProfiling();
  }

  void InitializeEventSet(int64 resolution_usec) {
    event_set_.reset(new activity::CallGraphTimelineEventSet(resolution_usec));
  }

  void StopProfiling() {
    if (profile_->profiling()) {
      profile_->Stop();
    }
  }

  void AppendTrace() {
    /* Append the following simple call tree:
          1
         /
        2
       /
      3
      Profile start time: 0usec

      Node 1 entry time:  1usec
      Node 1 exit time:   6usec
      Node 1 total time:  5usec
      Node 1 own time:    2usec (1-2, 5-6)

      Node 2 entry time:  2usec
      Node 2 exit time:   5usec
      Node 2 total time:  3usec
      Node 2 own time:    2usec (2-3, 4-5)

      Node 3 entry time:  3usec
      Node 3 exit time:   4usec
      Node 3 total time:  1usec
      Node 3 own time:    1usec (3-4)
    */

    profile_->OnFunctionEntry(function_info_1_.get());
    profile_->OnFunctionEntry(function_info_2_.get());
    profile_->OnFunctionEntry(function_info_3_.get());
    profile_->OnFunctionExit(function_info_3_.get());
    profile_->OnFunctionExit(function_info_2_.get());
    profile_->OnFunctionExit(function_info_1_.get());
  }

  void AppendEventsToVector() {
    const activity::CallGraphTimelineEventSet::EventMap *event_map =
        event_set_->event_map();

    for (activity::CallGraphTimelineEventSet::EventMap::const_iterator iter =
             event_map->begin(), end = event_map->end();
         iter != end;
         ++iter) {
      event_vector_.push_back(iter->second);
    }
  }

  // These assertions are common to the tests for CallGraphTimeline
  // and CallGraphTimelineVisitor, so we put them here.
  void AssertBasicTest(const int64 resolution_usec) {
    ASSERT_EQ(8, event_vector_.size());

    int zero_intensity_event_counter = 0;
    const int64 profile_entry_time =
        profile_->profile()->call_tree(0).entry_time_usec();
    for (int i = 0; i < event_vector_.size(); ++i) {
      activity::CallGraphTimelineEvent *event = event_vector_[i];
      ASSERT_EQ(resolution_usec, event->duration_usec);
      ASSERT_EQ(activity::CallGraphTimelineEvent::JS_EXECUTE, event->type);
      if (event->intensity == 0) {
        zero_intensity_event_counter++;
        continue;
      }
      ASSERT_EQ(profile_entry_time + (i - zero_intensity_event_counter),
                event->start_time_usec);
      ASSERT_EQ(1, event->intensity);
    }
    ASSERT_EQ(3, zero_intensity_event_counter);

    ASSERT_STREQ(kTestUrl1, event_vector_[0]->identifier);
    ASSERT_STREQ(kTestUrl2, event_vector_[1]->identifier);
    ASSERT_STREQ(kTestUrl1, event_vector_[2]->identifier);
    ASSERT_STREQ(kTestUrl2, event_vector_[3]->identifier);
    ASSERT_STREQ(kTestUrl1, event_vector_[4]->identifier);
    ASSERT_STREQ(kTestUrl2, event_vector_[5]->identifier);
    ASSERT_STREQ(kTestUrl1, event_vector_[6]->identifier);
    ASSERT_STREQ(kTestUrl1, event_vector_[7]->identifier);
  }

  void AssertPartialTest(const int64 resolution_usec) {
    ASSERT_EQ(5, event_vector_.size());

    int zero_intensity_event_counter = 0;
    const int64 profile_entry_time =
        profile_->profile()->call_tree(0).entry_time_usec();
    for (int i = 0; i < event_vector_.size(); ++i) {
      activity::CallGraphTimelineEvent *event = event_vector_[i];
      ASSERT_EQ(resolution_usec, event->duration_usec);
      ASSERT_EQ(activity::CallGraphTimelineEvent::JS_EXECUTE, event->type);
      if (event->intensity == 0) {
        zero_intensity_event_counter++;
        continue;
      }
      ASSERT_EQ(profile_entry_time + 2LL + (i - zero_intensity_event_counter),
                event->start_time_usec);
      ASSERT_EQ(1, event->intensity);
    }
    ASSERT_EQ(2, zero_intensity_event_counter);

    ASSERT_STREQ(kTestUrl2, event_vector_[0]->identifier);
    ASSERT_STREQ(kTestUrl1, event_vector_[1]->identifier);
    ASSERT_STREQ(kTestUrl2, event_vector_[2]->identifier);
    ASSERT_STREQ(kTestUrl1, event_vector_[3]->identifier);
    ASSERT_STREQ(kTestUrl1, event_vector_[4]->identifier);
  }

  void AssertLowResolutionTest(
      const int64 start_time_usec, const int64 resolution_usec) {
    ASSERT_EQ(4, event_vector_.size());

    for (int i = 0; i < event_vector_.size(); ++i) {
      ASSERT_EQ(resolution_usec, event_vector_[i]->duration_usec);
      ASSERT_EQ(
          activity::CallGraphTimelineEvent::JS_EXECUTE, event_vector_[i]->type);
    }

    ASSERT_EQ(start_time_usec, event_vector_[0]->start_time_usec);
    ASSERT_EQ(start_time_usec, event_vector_[1]->start_time_usec);
    ASSERT_EQ(start_time_usec + 3, event_vector_[2]->start_time_usec);
    ASSERT_EQ(start_time_usec + 3, event_vector_[3]->start_time_usec);

    ASSERT_STREQ(kTestUrl2, event_vector_[0]->identifier);
    ASSERT_STREQ(kTestUrl1, event_vector_[1]->identifier);
    ASSERT_STREQ(kTestUrl2, event_vector_[2]->identifier);
    ASSERT_STREQ(kTestUrl1, event_vector_[3]->identifier);

    // node 2 runs from 2usec to 3usec
    ASSERT_EQ(1, event_vector_[0]->intensity);
    // node 1 runs from 1usec to 2usec
    ASSERT_EQ(1, event_vector_[1]->intensity);
    // node 2 runs from 4usec to 5usec
    ASSERT_EQ(1, event_vector_[2]->intensity);
    // node 3 runs from 3usec to 4usec, node 1 runs from 5usec to 6usec
    ASSERT_EQ(2, event_vector_[3]->intensity);
  }

  scoped_ptr<activity_testing::MockClock> clock_;
  scoped_ptr<activity::CallGraphProfile> profile_;
  scoped_ptr<activity::FunctionInfoInterface> function_info_1_;
  scoped_ptr<activity::FunctionInfoInterface> function_info_2_;
  scoped_ptr<activity::FunctionInfoInterface> function_info_3_;
  scoped_ptr<activity::CallGraphTimelineEventSet> event_set_;
  std::vector<activity::CallGraphTimelineEvent*> event_vector_;
};

}  // namespace activity_testing

#endif  // CALL_GRAPH_TIMELINE_TEST_BASE_H_
