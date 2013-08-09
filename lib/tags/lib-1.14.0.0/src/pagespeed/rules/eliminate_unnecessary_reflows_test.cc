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

#include "pagespeed/proto/pagespeed_output.pb.h"
#include "pagespeed/proto/timeline.pb.h"
#include "pagespeed/rules/eliminate_unnecessary_reflows.h"
#include "pagespeed/testing/instrumentation_data_builder.h"
#include "pagespeed/testing/pagespeed_test.h"

namespace {

using pagespeed::EliminateUnnecessaryReflowsDetails;
using pagespeed::rules::EliminateUnnecessaryReflows;
using pagespeed_testing::InstrumentationDataBuilder;

class EliminateUnnecessaryReflowsTest
    : public pagespeed_testing::PagespeedRuleTest<EliminateUnnecessaryReflows> {
 protected:
  virtual void DoSetUp() {
    NewScriptResource(kUrl1);
    NewScriptResource(kUrl2);
  }

  InstrumentationDataBuilder builder_;
};

TEST_F(EliminateUnnecessaryReflowsTest, Basic) {
  AddInstrumentationData(builder_
                         .ParseHTML(0, 0, 0)
                         .EvaluateScript(kUrl1, 0)
                         .Layout()
                         .Layout()
                         .AddFrame(kUrl2, 1, 2, "funcName")
                         .Get());
  Freeze();
  AppendResults();

  ASSERT_EQ(1, num_results());
  ASSERT_EQ(kUrl1, result(0).resource_urls(0));
  ASSERT_EQ(1, result(0).savings().page_reflows_saved());
  const EliminateUnnecessaryReflowsDetails& detail =
      details<EliminateUnnecessaryReflowsDetails>(0);
  ASSERT_EQ(1, detail.stack_trace_size());
  ASSERT_EQ(1, detail.stack_trace(0).count());
  ASSERT_EQ(1, detail.stack_trace(0).frame_size());
  ASSERT_EQ(kUrl2, detail.stack_trace(0).frame(0).url());
  ASSERT_EQ(1, detail.stack_trace(0).frame(0).line_number());
  ASSERT_EQ(2, detail.stack_trace(0).frame(0).column_number());
  ASSERT_EQ("funcName", detail.stack_trace(0).frame(0).function_name());
}

TEST_F(EliminateUnnecessaryReflowsTest, MissingResourceIgnored) {
  AddInstrumentationData(builder_
                         .ParseHTML(0, 0, 0)
                         .EvaluateScript(kUrl3, 0)
                         .Layout()
                         .AddFrame(kUrl2, 1, 2, "funcName")
                         .Get());
  Freeze();
  AppendResults();

  ASSERT_EQ(0, num_results());
}

TEST_F(EliminateUnnecessaryReflowsTest, NoResultForLayoutWithoutStack) {
  AddInstrumentationData(builder_
                         .ParseHTML(0, 0, 0)
                         .EvaluateScript(kUrl1, 0)
                         .Layout()
                         .Get());
  Freeze();
  AppendResults();

  ASSERT_EQ(0, num_results());
}

TEST_F(EliminateUnnecessaryReflowsTest, NoResultTopLevelLayouts) {
  AddInstrumentationData(builder_.Layout().Get());
  AddInstrumentationData(builder_.Layout().Get());
  AddInstrumentationData(builder_.Layout().Get());
  Freeze();
  AppendResults();

  ASSERT_EQ(0, num_results());
}

TEST_F(EliminateUnnecessaryReflowsTest, AggregateUniqueStackTraces) {
  AddInstrumentationData(builder_
                         .ParseHTML(0, 0, 0)
                         .EvaluateScript(kUrl1, 0)
                         .Layout()
                         .AddFrame(kUrl2, 1, 2, "funcName")
                         .AddFrame(kUrl2, 3, 4, "otherFunc")
                         .Pop()
                         .Layout()
                         .AddFrame(kUrl2, 1, 2, "funcName")
                         .AddFrame(kUrl2, 3, 4, "otherFunc")
                         .Pop()
                         .Layout()
                         .AddFrame(kUrl2, 1, 2, "funcName")
                         .AddFrame(kUrl2, 3, 4, "otherFunc")
                         .Pop()
                         .Layout()
                         .AddFrame(kUrl2, 1, 2, "funcName")
                         .AddFrame(kUrl2, 3, 5, "otherFunc")
                         .Get());
  AddInstrumentationData(builder_
                         .ParseHTML(0, 0, 0)
                         .EvaluateScript(kUrl1, 0)
                         .Layout()
                         .AddFrame(kUrl2, 1, 2, "funcName")
                         .AddFrame(kUrl2, 3, 5, "otherFunc")
                         .Get());
  Freeze();
  AppendResults();

  ASSERT_EQ(1, num_results());
  ASSERT_EQ(kUrl1, result(0).resource_urls(0));
  ASSERT_EQ(5, result(0).savings().page_reflows_saved());
  const EliminateUnnecessaryReflowsDetails& detail =
      details<EliminateUnnecessaryReflowsDetails>(0);
  ASSERT_EQ(2, detail.stack_trace_size());

  const pagespeed::EliminateUnnecessaryReflowsDetails_StackTrace& trace1 =
      detail.stack_trace(0);
  ASSERT_EQ(3, trace1.count());
  ASSERT_EQ(2, trace1.frame_size());
  ASSERT_EQ(kUrl2, trace1.frame(0).url());
  ASSERT_EQ(1, trace1.frame(0).line_number());
  ASSERT_EQ(2, trace1.frame(0).column_number());
  ASSERT_EQ("funcName", trace1.frame(0).function_name());
  ASSERT_EQ(kUrl2, trace1.frame(1).url());
  ASSERT_EQ(3, trace1.frame(1).line_number());
  ASSERT_EQ(4, trace1.frame(1).column_number());
  ASSERT_EQ("otherFunc", trace1.frame(1).function_name());

  const pagespeed::EliminateUnnecessaryReflowsDetails_StackTrace& trace2 =
      detail.stack_trace(1);
  ASSERT_EQ(2, trace2.count());
  ASSERT_EQ(2, trace2.frame_size());
  ASSERT_EQ(kUrl2, trace2.frame(0).url());
  ASSERT_EQ(1, trace2.frame(0).line_number());
  ASSERT_EQ(2, trace2.frame(0).column_number());
  ASSERT_EQ("funcName", trace2.frame(0).function_name());
  ASSERT_EQ(kUrl2, trace2.frame(1).url());
  ASSERT_EQ(3, trace2.frame(1).line_number());
  ASSERT_EQ(5, trace2.frame(1).column_number());
  ASSERT_EQ("otherFunc", trace2.frame(1).function_name());
}

}  // namespace
