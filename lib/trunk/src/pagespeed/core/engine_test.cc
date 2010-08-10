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

#include <string>
#include <vector>

#include "pagespeed/core/engine.h"
#include "pagespeed/core/pagespeed_input.h"
#include "pagespeed/core/result_provider.h"
#include "pagespeed/core/rule.h"
#include "pagespeed/proto/pagespeed_output.pb.h"
#include "pagespeed/formatters/proto_formatter.h"
#include "testing/gtest/include/gtest/gtest.h"

using pagespeed::Argument;
using pagespeed::Engine;
using pagespeed::FormatArgument;
using pagespeed::Formatter;
using pagespeed::InputInformation;
using pagespeed::PagespeedInput;
using pagespeed::Result;
using pagespeed::Results;
using pagespeed::ResultProvider;
using pagespeed::ResultText;
using pagespeed::Rule;
using pagespeed::RuleFormatter;
using pagespeed::formatters::ProtoFormatter;

namespace {

const char* kRuleName = "TestRule";
const char* kHeader = "Test Rule";
const char* kDocumentationUrl = "foobar.html#TestRule";
const char* kBody1 = "Example format string";
const char* kBody2 = "Another format string";

class TestRule : public Rule {
 public:
  TestRule() : append_results_return_value_(true) {}
  virtual ~TestRule() {}

  virtual const char* name() const {
    return kRuleName;
  }

  // Human readable rule name.
  virtual const char* header() const {
    return kHeader;
  }

  virtual const char* documentation_url() const {
    return kDocumentationUrl;
  }

  void set_append_results_return_value(bool retval) {
    append_results_return_value_ = retval;
  }

  virtual bool AppendResults(const PagespeedInput& input,
                             ResultProvider* provider) {
    provider->NewResult();
    return append_results_return_value_;
  }

  virtual void FormatResults(const pagespeed::ResultVector& results,
                             Formatter* formatter) {
    formatter->AddChild(kBody1);
    formatter->AddChild(kBody2);
  }

 private:
  bool append_results_return_value_;

  DISALLOW_COPY_AND_ASSIGN(TestRule);
};

TEST(EngineTest, ComputeResults) {
  PagespeedInput input;
  input.Freeze();

  std::vector<Rule*> rules;
  rules.push_back(new TestRule());

  Engine engine(&rules);
  engine.Init();
  Results results;
  ASSERT_TRUE(engine.ComputeResults(input, &results));
  ASSERT_EQ(1, results.results_size());
  ASSERT_EQ(1, results.rules_size());
  ASSERT_EQ(kRuleName, results.rules(0));
  ASSERT_EQ(0, results.error_rules_size());
  ASSERT_NE(0, results.version().major());
  ASSERT_NE(0, results.version().minor());

  const Result& result = results.results(0);
  EXPECT_EQ(result.rule_name(), kRuleName);
}

TEST(EngineTest, ComputeResultsError) {
  PagespeedInput input;
  input.Freeze();

  std::vector<Rule*> rules;
  TestRule* rule = new TestRule();
  rule->set_append_results_return_value(false);
  rules.push_back(rule);

  Engine engine(&rules);
  engine.Init();
  Results results;
  ASSERT_FALSE(engine.ComputeResults(input, &results));
  ASSERT_EQ(1, results.results_size());
  ASSERT_EQ(1, results.rules_size());
  ASSERT_EQ(1, results.error_rules_size());
  ASSERT_EQ(kRuleName, results.rules(0));
  ASSERT_EQ(kRuleName, results.error_rules(0));

  const Result& result = results.results(0);
  EXPECT_EQ(result.rule_name(), kRuleName);
}

TEST(EngineTest, FormatResults) {
  PagespeedInput input;
  input.Freeze();

  std::vector<Rule*> rules;
  rules.push_back(new TestRule());

  Engine engine(&rules);
  engine.Init();
  Results results;
  ASSERT_TRUE(engine.ComputeResults(input, &results));

  std::vector<ResultText*> result_text;
  ProtoFormatter formatter(&result_text);
  ASSERT_TRUE(engine.FormatResults(results, &formatter));
  ASSERT_EQ(static_cast<size_t>(1), result_text.size());
  const ResultText& root = *result_text[0];
  ASSERT_EQ(kHeader, root.format());
  ASSERT_EQ(0, root.args_size());
  ASSERT_EQ(2, root.children_size());
  ASSERT_EQ(kBody1, root.children(0).format());
  ASSERT_EQ(kBody2, root.children(1).format());
  delete result_text[0];
}

TEST(EngineTest, FormatResultsNoResults) {
  PagespeedInput input;
  input.Freeze();

  std::vector<Rule*> rules;
  rules.push_back(new TestRule());

  Engine engine(&rules);
  engine.Init();
  Results results;
  ASSERT_TRUE(engine.ComputeResults(input, &results));
  ASSERT_EQ(1, results.rules_size());
  ASSERT_EQ(1, results.results_size());
  results.clear_results();
  ASSERT_EQ(0, results.results_size());

  ASSERT_EQ(1, results.rules_size());

  // Verify that when there are no results, but there is an entry in
  // the rules vector, we do emit a header for that rule.
  std::vector<ResultText*> result_text;
  ProtoFormatter formatter(&result_text);
  ASSERT_TRUE(engine.FormatResults(results, &formatter));
  ASSERT_EQ(static_cast<size_t>(1), result_text.size());
  const ResultText& root = *result_text[0];
  ASSERT_EQ(kHeader, root.format());
  ASSERT_EQ(0, root.args_size());
  ASSERT_EQ(0, root.children_size());
  delete result_text[0];
}

TEST(EngineTest, FormatResultsEngineNotInitialized) {
  TestRule rule;
  Results results;
  Result* result = results.add_results();
  result->set_rule_name(rule.name());

  std::vector<Rule*> rules;
  rules.push_back(new TestRule());
  Engine engine(&rules);

  std::vector<ResultText*> result_text;
  ProtoFormatter formatter(&result_text);
  ASSERT_DEATH(engine.FormatResults(results, &formatter),
               "Check failed: init_.");
}

TEST(EngineTest, FormatResultsNotInitialized) {
  Results results;
  std::vector<Rule*> rules;
  rules.push_back(new TestRule());
  Engine engine(&rules);
  engine.Init();

  std::vector<ResultText*> result_text;
  ProtoFormatter formatter(&result_text);
  ASSERT_FALSE(engine.FormatResults(results, &formatter));
}

TEST(EngineTest, FormatResultsNoRuleInstance) {
  PagespeedInput input;
  input.Freeze();

  std::vector<Rule*> rules;
  rules.push_back(new TestRule());

  Engine engine(&rules);
  engine.Init();
  Results results;
  ASSERT_TRUE(engine.ComputeResults(input, &results));
  ASSERT_EQ(1, results.results_size());

  // Now instantiate an Engine with no Rules and attempt to format the
  // results. We expect this to fail since the Engine doesn't know
  // about the Rule in the Results structure.
  rules.clear();
  Engine engine2(&rules);
  engine2.Init();

  std::vector<ResultText*> result_text;
  ProtoFormatter formatter(&result_text);
  ASSERT_FALSE(engine2.FormatResults(results, &formatter));
  ASSERT_EQ(static_cast<size_t>(0), result_text.size());
}

TEST(Engine, NonFrozenInputFails) {
  PagespeedInput input;
  std::vector<Rule*> rules;
  rules.push_back(new TestRule());

  Engine engine(&rules);
  engine.Init();
  Results results;
#ifdef NDEBUG
  ASSERT_FALSE(engine.ComputeResults(input, &results));
  ASSERT_EQ(0, results.results_size());
#else
  ASSERT_DEATH(engine.ComputeResults(input, &results),
               "Attempting to ComputeResults with non-frozen input.");
#endif
}

}  // namespace
