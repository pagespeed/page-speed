// Copyright 2009 Google Inc. All Rights Reserved.
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

#include "pagespeed/core/engine.h"
#include "pagespeed/core/pagespeed_input.h"
#include "pagespeed/core/pagespeed_output.pb.h"
#include "pagespeed/core/rule.h"
#include "testing/gtest/include/gtest/gtest.h"

using pagespeed::Engine;
using pagespeed::PagespeedInput;
using pagespeed::Result;
using pagespeed::Results;
using pagespeed::ResultText;
using pagespeed::Rule;

namespace {

class TestRule : public Rule {
 public:
  TestRule() {}
  virtual ~TestRule() {}

  virtual bool AppendResults(const PagespeedInput& input, Results* results) {
    Result* result = results->add_results();
    result->set_rule_name("TestRule");
    return true;
  }

  virtual void InterpretResults(const Results& results,
                                ResultText* result_text) {
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(TestRule);
};

TEST(EngineTest, BasicTest) {
  PagespeedInput input;

  std::vector<Rule*> rules;
  rules.push_back(new TestRule());

  Engine engine(rules);
  Results results;
  engine.GetResults(input, &results);
  ASSERT_EQ(results.results_size(), 1);

  const Result& result = results.results(0);
  EXPECT_EQ(result.rule_name(), "TestRule");
}

}  // namespace
