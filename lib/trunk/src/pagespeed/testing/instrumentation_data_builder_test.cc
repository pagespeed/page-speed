// Copyright 2011 Google Inc. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "base/memory/scoped_ptr.h"
#include "pagespeed/proto/timeline.pb.h"
#include "pagespeed/testing/instrumentation_data_builder.h"
#include "testing/gtest/include/gtest/gtest.h"

using pagespeed_testing::InstrumentationDataBuilder;

TEST(InstrumentationDataBuilderTest, Basic) {
  InstrumentationDataBuilder b;
  scoped_ptr<pagespeed::InstrumentationData> d(
      // Create a layout event with a JS stack of 3 frames.
      b.Layout()
      .AddFrame("http://www.example.com/", 0, 1, "funcName")
      .AddFrame("http://www.example.com/", 1, 2, "otherFunc")
      .AddFrame("http://www.example.com/foo.js", 2, 3, "thirdFunc")

      // Create an evaluate script event with no JS stack.
      .EvaluateScript("http://www.example.com/", 10)

      // Return to the parent node, so we can add more children to it.
      .Pop()

      // Create an evaluate script event with no JS stack.
      .EvaluateScript("http://www.example.com/foo.js", 20)

      // Create a layout event with a JS stack of 1 frame.
      .Layout().AddFrame("http://www.example.com/", 5, 6, "lastFunc")
      .Get());

  ASSERT_EQ(pagespeed::InstrumentationData_RecordType_LAYOUT, d->type());
  ASSERT_EQ(0, d->start_time());
  ASSERT_EQ(7, d->end_time());
  ASSERT_EQ(0, d->start_tick());
  ASSERT_EQ(7, d->end_tick());
  ASSERT_EQ(2, d->children_size());
  ASSERT_EQ(3, d->stack_trace_size());
  ASSERT_EQ("http://www.example.com/", d->stack_trace(0).url());
  ASSERT_EQ(0, d->stack_trace(0).line_number());
  ASSERT_EQ(1, d->stack_trace(0).column_number());
  ASSERT_EQ("funcName", d->stack_trace(0).function_name());
  ASSERT_EQ("http://www.example.com/", d->stack_trace(1).url());
  ASSERT_EQ(1, d->stack_trace(1).line_number());
  ASSERT_EQ(2, d->stack_trace(1).column_number());
  ASSERT_EQ("otherFunc", d->stack_trace(1).function_name());
  ASSERT_EQ("http://www.example.com/foo.js", d->stack_trace(2).url());
  ASSERT_EQ(2, d->stack_trace(2).line_number());
  ASSERT_EQ(3, d->stack_trace(2).column_number());
  ASSERT_EQ("thirdFunc", d->stack_trace(2).function_name());

  const pagespeed::InstrumentationData& child1 = d->children(0);
  ASSERT_EQ(pagespeed::InstrumentationData_RecordType_EVALUATE_SCRIPT,
            child1.type());
  ASSERT_EQ(1, child1.start_time());
  ASSERT_EQ(2, child1.end_time());
  ASSERT_EQ(1, child1.start_tick());
  ASSERT_EQ(2, child1.end_tick());
  ASSERT_EQ(0, child1.children_size());
  ASSERT_EQ(0, child1.stack_trace_size());
  ASSERT_EQ("http://www.example.com/", child1.data().url());
  ASSERT_EQ(10, child1.data().line_number());

  const pagespeed::InstrumentationData& child2 = d->children(1);
  ASSERT_EQ(pagespeed::InstrumentationData_RecordType_EVALUATE_SCRIPT,
            child2.type());
  ASSERT_EQ(3, child2.start_time());
  ASSERT_EQ(6, child2.end_time());
  ASSERT_EQ(3, child2.start_tick());
  ASSERT_EQ(6, child2.end_tick());
  ASSERT_EQ(1, child2.children_size());
  ASSERT_EQ(0, child2.stack_trace_size());
  ASSERT_EQ("http://www.example.com/foo.js", child2.data().url());
  ASSERT_EQ(20, child2.data().line_number());

  const pagespeed::InstrumentationData& child3 = child2.children(0);
  ASSERT_EQ(pagespeed::InstrumentationData_RecordType_LAYOUT, child3.type());
  ASSERT_EQ(4, child3.start_time());
  ASSERT_EQ(5, child3.end_time());
  ASSERT_EQ(4, child3.start_tick());
  ASSERT_EQ(5, child3.end_tick());
  ASSERT_EQ(0, child3.children_size());
  ASSERT_EQ(1, child3.stack_trace_size());
  ASSERT_EQ("http://www.example.com/", child3.stack_trace(0).url());
  ASSERT_EQ(5, child3.stack_trace(0).line_number());
  ASSERT_EQ(6, child3.stack_trace(0).column_number());
  ASSERT_EQ("lastFunc", child3.stack_trace(0).function_name());
}

TEST(InstrumentationDataBuilderTest, Reuse) {
  InstrumentationDataBuilder b;
  ASSERT_EQ(NULL, b.Get());

  scoped_ptr<pagespeed::InstrumentationData> d(b.Layout().Get());
  ASSERT_EQ(pagespeed::InstrumentationData_RecordType_LAYOUT, d->type());
  ASSERT_EQ(0, d->start_time());
  ASSERT_EQ(1, d->end_time());
  ASSERT_EQ(0, d->start_tick());
  ASSERT_EQ(1, d->end_tick());

  ASSERT_EQ(NULL, b.Get());

  d.reset(b.Layout().Get());
  ASSERT_EQ(pagespeed::InstrumentationData_RecordType_LAYOUT, d->type());
  ASSERT_EQ(2, d->start_time());
  ASSERT_EQ(3, d->end_time());
  ASSERT_EQ(2, d->start_tick());
  ASSERT_EQ(3, d->end_tick());

  ASSERT_EQ(NULL, b.Get());
}

TEST(InstrumentationDataBuilderTest, ReplaceStackFails) {
  InstrumentationDataBuilder b;
  // Add a root event, then pop it.
  b.Layout();
  b.Pop();

  //  At this point there's nothing left on the working set
  //  stack. Attempting to add a new event should fail.
  ASSERT_DEATH(b.Layout(), "Unable to add new event to empty working set.");
}
