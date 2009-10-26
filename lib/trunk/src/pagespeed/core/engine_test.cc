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
#include "pagespeed/core/rule.h"
#include "pagespeed/proto/pagespeed_output.pb.h"
#include "pagespeed/formatters/proto_formatter.h"
#include "testing/gtest/include/gtest/gtest.h"

using pagespeed::Argument;
using pagespeed::Engine;
using pagespeed::Formatter;
using pagespeed::PagespeedInput;
using pagespeed::Result;
using pagespeed::Results;
using pagespeed::ResultText;
using pagespeed::Rule;
using pagespeed::formatters::ProtoFormatter;

namespace {

const char* kHeader = "Test Rule";
const char* kBody1 = "Example format string";
const char* kBody2 = "Another format string";

class TestRule : public Rule {
 public:
  TestRule() : Rule("TestRule") {}
  virtual ~TestRule() {}

  virtual bool AppendResults(const PagespeedInput& input, Results* results) {
    Result* result = results->add_results();
    result->set_rule_name(name());
    return true;
  }

  virtual void FormatResults(const ResultVector& results,
                             Formatter* formatter) {
    Formatter* header = formatter->AddChild(kHeader);
    Formatter* body = header->AddChild(kBody1);
    Formatter* body2 = header->AddChild(kBody2);
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(TestRule);
};

TEST(EngineTest, ComputeResults) {
  PagespeedInput input;

  std::vector<Rule*> rules;
  rules.push_back(new TestRule());

  Engine engine(rules);
  engine.Init();
  Results results;
  ASSERT_TRUE(engine.ComputeResults(input, &results));
  ASSERT_EQ(results.results_size(), 1);

  const Result& result = results.results(0);
  EXPECT_EQ(result.rule_name(), "TestRule");
}

TEST(EngineTest, FormatResults) {
  TestRule rule;
  Results results;
  Result* result = results.add_results();
  result->set_rule_name(rule.name());

  std::vector<Rule*> rules;
  rules.push_back(new TestRule());
  Engine engine(rules);
  engine.Init();

  std::vector<ResultText*> result_text;
  ProtoFormatter formatter(&result_text);
  ASSERT_TRUE(engine.FormatResults(results, &formatter));
  ASSERT_EQ(1, result_text.size());
  const ResultText& root = *result_text[0];
  ASSERT_EQ(kHeader, root.format());
  ASSERT_EQ(2, root.children_size());
  ASSERT_EQ(kBody1, root.children(0).format());
  ASSERT_EQ(kBody2, root.children(1).format());
  delete result_text[0];
}

TEST(EngineTest, FormatResultsNoInitFails) {
  TestRule rule;
  Results results;
  Result* result = results.add_results();
  result->set_rule_name(rule.name());

  std::vector<Rule*> rules;
  rules.push_back(new TestRule());
  Engine engine(rules);

  std::vector<ResultText*> result_text;
  ProtoFormatter formatter(&result_text);
  ASSERT_DEATH(engine.FormatResults(results, &formatter),
               "Check failed: init_.");
}

TEST(EngineTest, FormatResultsNoRuleInstance) {
  TestRule rule;
  Results results;
  Result* result = results.add_results();
  result->set_rule_name("NoSuchRule");

  std::vector<Rule*> rules;
  rules.push_back(new TestRule());
  Engine engine(rules);
  engine.Init();

  std::vector<ResultText*> result_text;
  ProtoFormatter formatter(&result_text);
  ASSERT_FALSE(engine.FormatResults(results, &formatter));
  ASSERT_EQ(0, result_text.size());
}

}  // namespace
