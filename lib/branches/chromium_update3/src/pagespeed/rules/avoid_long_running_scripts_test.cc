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

#include <string>

#include "base/memory/scoped_ptr.h"
#include "pagespeed/proto/pagespeed_output.pb.h"
#include "pagespeed/rules/avoid_long_running_scripts.h"
#include "pagespeed/testing/instrumentation_data_builder.h"
#include "pagespeed/testing/pagespeed_test.h"

using ::pagespeed_testing::PagespeedRuleTest;

namespace {

class AvoidLongRunningScriptsTest
    : public PagespeedRuleTest<pagespeed::rules::AvoidLongRunningScripts> {
 protected:
  virtual void DoSetUp() {
    NewScriptResource(kUrl1);
    NewScriptResource(kUrl2);
  }

  ::pagespeed_testing::InstrumentationDataBuilder builder_;
};

TEST_F(AvoidLongRunningScriptsTest, NoScript) {
  AddInstrumentationData(builder_
                         .ParseHTML(0, 0, 0)
                         .Layout()
                         .Get());
  Freeze();
  AppendResults();

  ASSERT_EQ(0, num_results());
}

TEST_F(AvoidLongRunningScriptsTest, ShortDuration) {
  AddInstrumentationData(builder_
                         .ParseHTML(0, 0, 0)
                         .EvaluateScript(kUrl1, 14)
                         .Pause(10.0)
                         .Pop()
                         .Layout()
                         .Get());
  Freeze();
  AppendResults();

  ASSERT_EQ(0, num_results());
}

TEST_F(AvoidLongRunningScriptsTest, LongDurationEvaluateScript) {
  AddInstrumentationData(builder_
                         .ParseHTML(0, 0, 0)
                         .EvaluateScript(kUrl1, 14)
                         .Pause(249.0)
                         .Pop()
                         .Layout()
                         .Get());
  Freeze();
  AppendResults();

  ASSERT_EQ(1, num_results());
  EXPECT_EQ(kUrl1, result(0).resource_urls(0));
  const pagespeed::AvoidLongRunningScriptsDetails& detail =
      details<pagespeed::AvoidLongRunningScriptsDetails>(0);
  EXPECT_EQ(14, detail.line_number());
  EXPECT_EQ(250.0, detail.duration_millis());
}

TEST_F(AvoidLongRunningScriptsTest, LongDurationFunctionCall) {
  AddInstrumentationData(builder_
                         .ParseHTML(0, 0, 0)
                         .FunctionCall(kUrl1, 14)
                         .Pause(249.0)
                         .Pop()
                         .Layout()
                         .Get());
  Freeze();
  AppendResults();

  ASSERT_EQ(1, num_results());
  EXPECT_EQ(kUrl1, result(0).resource_urls(0));
  const pagespeed::AvoidLongRunningScriptsDetails& detail =
      details<pagespeed::AvoidLongRunningScriptsDetails>(0);
  EXPECT_EQ(14, detail.line_number());
  EXPECT_EQ(250.0, detail.duration_millis());
}

}  // namespace
