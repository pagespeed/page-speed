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

#include <vector>

#include "base/stl_util-inl.h"
#include "call_graph_timeline_event.h"
#include "call_graph_timeline_event_set.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace {

const char* kTestUrl1 = "http://foo.com/index.html";
const char* kTestUrl2 = "http://bar.com/index.html";
const int64 kDurationUsec = 10LL;

class CallGraphTimelineEventSetTest : public testing::Test {
 protected:
  CallGraphTimelineEventSetTest() : events_(kDurationUsec) {}

  virtual void SetUp() {
    event1_ = events_.GetOrCreateEvent(
        kTestUrl1, activity::CallGraphTimelineEvent::JS_PARSE, 0);
    event2_ = events_.GetOrCreateEvent(
        kTestUrl1, activity::CallGraphTimelineEvent::JS_EXECUTE, 0);
    event3_ = events_.GetOrCreateEvent(
        kTestUrl2, activity::CallGraphTimelineEvent::JS_PARSE, 0);
    event4_ = events_.GetOrCreateEvent(
        kTestUrl2, activity::CallGraphTimelineEvent::JS_EXECUTE, 10);
    event1_again_ = events_.GetOrCreateEvent(
        kTestUrl1, activity::CallGraphTimelineEvent::JS_PARSE, 0);
  }

  activity::CallGraphTimelineEventSet events_;
  activity::CallGraphTimelineEvent *event1_;
  activity::CallGraphTimelineEvent *event2_;
  activity::CallGraphTimelineEvent *event3_;
  activity::CallGraphTimelineEvent *event4_;
  activity::CallGraphTimelineEvent *event1_again_;
};

TEST_F(CallGraphTimelineEventSetTest, GetOrCreateEvent) {
  ASSERT_EQ(event1_, event1_again_)
      << "GetOrCreateEvent returned different instances for the same event";
  ASSERT_NE(event1_, event2_)
      << "GetOrCreateEvent returned the same instance for different events";
  ASSERT_NE(event1_, event3_)
      << "GetOrCreateEvent returned the same instance for different events";
  ASSERT_NE(event2_, event3_)
      << "GetOrCreateEvent returned the same instance for different events";
}

TEST_F(CallGraphTimelineEventSetTest, ValidateMapValues) {
  const activity::CallGraphTimelineEventSet::EventMap *event_map =
      events_.event_map();

  std::vector<const activity::CallGraphTimelineEvent*> event_vector;
  for (activity::CallGraphTimelineEventSet::EventMap::const_iterator iter =
           event_map->begin(), end = event_map->end();
       iter != end;
       ++iter) {
    event_vector.push_back(iter->second);
  }

  ASSERT_EQ(4, event_vector.size());
  ASSERT_EQ(event3_, event_vector[0]);
  ASSERT_EQ(event1_, event_vector[1]);
  ASSERT_EQ(event2_, event_vector[2]);
  ASSERT_EQ(event4_, event_vector[3]);
}

}  // namespace
