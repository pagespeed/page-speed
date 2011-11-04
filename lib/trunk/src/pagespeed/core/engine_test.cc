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
#include "pagespeed/core/rule_input.h"
#include "pagespeed/formatters/proto_formatter.h"
#include "pagespeed/l10n/l10n.h"
#include "pagespeed/l10n/localizer.h"
#include "pagespeed/proto/pagespeed_output.pb.h"
#include "pagespeed/proto/pagespeed_proto_formatter.pb.h"
#include "pagespeed/testing/pagespeed_test.h"

using pagespeed::AlwaysAcceptResultFilter;
using pagespeed::Engine;
using pagespeed::FormatArgument;
using pagespeed::Formatter;
using pagespeed::InputInformation;
using pagespeed::UserFacingString;
using pagespeed::FormattedResults;
using pagespeed::FormattedRuleResults;
using pagespeed::PagespeedInput;
using pagespeed::Result;
using pagespeed::ResultProvider;
using pagespeed::Results;
using pagespeed::Rule;
using pagespeed::RuleFormatter;
using pagespeed::RuleInput;
using pagespeed::RuleResults;
using pagespeed::formatters::ProtoFormatter;
using pagespeed::l10n::NullLocalizer;

namespace {

const char* kRuleName = "TestRule";
const char* kExperimentalRuleName = "TestExperimentalRule";
const char* kHeader = "Test Rule";
const char* kBody1 = "Example format string";
const char* kBody2 = "Another format string";

class TestRule : public Rule {
 public:
  explicit TestRule(const char* name = kRuleName)
      : pagespeed::Rule(pagespeed::InputCapabilities()),
        name_(name),
        append_results_return_value_(true),
        append_results_(true),
        score_(100),
        impact_(0.25) {}
  virtual ~TestRule() {}

  virtual const char* name() const {
    return name_;
  }

  // Human readable rule name.
  virtual UserFacingString header() const {
    return not_localized(kHeader);
  }

  void set_append_results_return_value(bool retval) {
    append_results_return_value_ = retval;
  }

  void set_append_results(bool append) { append_results_ = append; }

  virtual bool AppendResults(const RuleInput& input,
                             ResultProvider* provider) {
    if (append_results_)
      provider->NewResult();
    return append_results_return_value_;
  }

  virtual void FormatResults(const pagespeed::ResultVector& results,
                             RuleFormatter* formatter) {
    formatter->AddUrlBlock(not_localized(kBody1));
    formatter->AddUrlBlock(not_localized(kBody2));
  }

  virtual double ComputeResultImpact(const InputInformation& input_info,
                                     const Result& result) {
    return impact_;
  }

  void set_score(int score) { score_ = score; }

  virtual int ComputeScore(const InputInformation& input_info,
                           const RuleResults& results) {
    return score_;
  }

 private:
  const char* name_;
  bool append_results_return_value_;
  bool append_results_;
  int score_;
  double impact_;

  DISALLOW_COPY_AND_ASSIGN(TestRule);
};

class TestExperimentalRule : public TestRule {
 public:
  explicit TestExperimentalRule(const char* name = kExperimentalRuleName)
      : TestRule(name) {}
  virtual bool IsExperimental() const { return true; }

 private:
  DISALLOW_COPY_AND_ASSIGN(TestExperimentalRule);
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
  ASSERT_EQ(1, results.rule_results_size());
  ASSERT_EQ(kRuleName, results.rule_results(0).rule_name());
  ASSERT_EQ(1, results.rule_results(0).results_size());
  ASSERT_EQ(100, results.rule_results(0).rule_score());
  ASSERT_EQ(0, results.error_rules_size());
  ASSERT_NE(0, results.version().major());
  ASSERT_NE(0, results.version().minor());
  ASSERT_EQ(75, results.score());

  const RuleResults& result = results.rule_results(0);
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
  ASSERT_EQ(1, results.rule_results_size());
  ASSERT_EQ(kRuleName, results.rule_results(0).rule_name());
  ASSERT_EQ(1, results.rule_results(0).results_size());
  ASSERT_EQ(1, results.error_rules_size());
  ASSERT_EQ(kRuleName, results.error_rules(0));
  ASSERT_TRUE(results.has_score());
  ASSERT_EQ(75, results.score());

  const RuleResults& result = results.rule_results(0);
  EXPECT_EQ(result.rule_name(), kRuleName);
}

TEST(EngineTest, NoScore) {
  PagespeedInput input;
  input.Freeze();

  TestRule* rule = new TestRule();
  std::vector<Rule*> rules;
  rule->set_score(-1);
  rules.push_back(rule);

  Engine engine(&rules);
  engine.Init();
  Results results;
  ASSERT_TRUE(engine.ComputeResults(input, &results));
  ASSERT_FALSE(results.rule_results(0).has_rule_score());
  ASSERT_FALSE(results.has_score());
}

TEST(EngineTest, ComputeScores) {
  PagespeedInput input;
  input.Freeze();

  std::vector<Rule*> rules;
  rules.push_back(new TestRule("rule1"));
  rules.push_back(new TestRule("rule2"));
  rules.push_back(new TestRule("rule3"));
  rules.push_back(new TestRule("rule4"));
  static_cast<TestRule*>(rules[0])->set_score(50);
  static_cast<TestRule*>(rules[1])->set_score(-1);
  static_cast<TestRule*>(rules[2])->set_score(120);  // should be clamped to 100
  static_cast<TestRule*>(rules[3])->set_score(100);
  static_cast<TestRule*>(rules[3])->set_append_results_return_value(false);

  Engine engine(&rules);
  engine.Init();
  Results results;
  ASSERT_FALSE(engine.ComputeResults(input, &results));

  ASSERT_EQ(50, results.rule_results(0).rule_score());
  ASSERT_FALSE(results.rule_results(1).has_rule_score());
  ASSERT_EQ(100, results.rule_results(2).rule_score());
  ASSERT_EQ(100, results.rule_results(3).rule_score());
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

  FormattedResults formatted_results;
  NullLocalizer localizer;
  ProtoFormatter formatter(&localizer, &formatted_results);

  ASSERT_TRUE(engine.FormatResults(results, &formatter));
  ASSERT_EQ(1, formatted_results.rule_results_size());
  const FormattedRuleResults& rule_results =
      formatted_results.rule_results(0);
  ASSERT_EQ(kHeader, rule_results.localized_rule_name());
  ASSERT_EQ(2, rule_results.url_blocks_size());
  ASSERT_EQ(kBody1, rule_results.url_blocks(0).header().format());
  ASSERT_EQ(kBody2, rule_results.url_blocks(1).header().format());
}

class NeverAcceptResultFilter : public pagespeed::ResultFilter {
 public:
  NeverAcceptResultFilter() {}
  virtual ~NeverAcceptResultFilter() {}

  virtual bool IsAccepted(const Result& result) const { return false; }

 private:
  DISALLOW_COPY_AND_ASSIGN(NeverAcceptResultFilter);
};

TEST(EngineTest, FormatResultsFilter) {
  PagespeedInput input;
  input.Freeze();

  std::vector<Rule*> rules;
  rules.push_back(new TestRule());

  Engine engine(&rules);
  engine.Init();
  Results results;
  ASSERT_TRUE(engine.ComputeResults(input, &results));
  results.set_score(50);
  RuleResults* rule_results = results.mutable_rule_results(0);
  rule_results->set_rule_score(50);
  rule_results->set_rule_impact(5.0);

  FormattedResults formatted_results;
  NullLocalizer localizer;
  ProtoFormatter formatter(&localizer, &formatted_results);
  NeverAcceptResultFilter filter;
  ASSERT_TRUE(engine.FormatResults(results, filter, &formatter));

  ASSERT_EQ(100, formatted_results.score());
  ASSERT_EQ(1, formatted_results.rule_results_size());
  const FormattedRuleResults& fmt_rule_results =
      formatted_results.rule_results(0);
  ASSERT_EQ(kHeader, fmt_rule_results.localized_rule_name());
  ASSERT_EQ(0, fmt_rule_results.url_blocks_size());
  ASSERT_EQ(100, fmt_rule_results.rule_score());
  ASSERT_EQ(0, fmt_rule_results.rule_impact());
}

TEST(EngineTest, FormatResultsNoResults) {
  PagespeedInput input;
  input.Freeze();

  TestRule* rule = new TestRule();
  rule->set_append_results(false);
  std::vector<Rule*> rules;
  rules.push_back(rule);

  Engine engine(&rules);
  engine.Init();
  Results results;
  ASSERT_TRUE(engine.ComputeResults(input, &results));
  ASSERT_EQ(1, results.rule_results_size());
  ASSERT_EQ(kRuleName, results.rule_results(0).rule_name());
  ASSERT_EQ(0, results.rule_results(0).results_size());

  // Verify that when there are no results, but there is an entry in
  // the rules vector, we do emit a header for that rule.
  FormattedResults formatted_results;
  NullLocalizer localizer;
  ProtoFormatter formatter(&localizer, &formatted_results);
  ASSERT_TRUE(engine.FormatResults(results, &formatter));

  ASSERT_EQ(1, formatted_results.rule_results_size());
  const FormattedRuleResults& rule_results =
      formatted_results.rule_results(0);
  ASSERT_EQ(kHeader, rule_results.localized_rule_name());
  ASSERT_EQ(0, rule_results.url_blocks_size());
}

TEST(EngineTest, FormatResultsEngineNotInitialized) {
  TestRule rule;
  Results results;
  RuleResults* rule_results = results.add_rule_results();
  rule_results->set_rule_name(rule.name());

  std::vector<Rule*> rules;
  rules.push_back(new TestRule());
  Engine engine(&rules);

  FormattedResults formatted_results;
  NullLocalizer localizer;
  ProtoFormatter formatter(&localizer, &formatted_results);
  ASSERT_DEATH(engine.FormatResults(results, &formatter),
               "Check failed: init_.");
}

TEST(EngineTest, FormatResultsNotInitialized) {
  Results results;
  std::vector<Rule*> rules;
  rules.push_back(new TestRule());
  Engine engine(&rules);
  engine.Init();

  FormattedResults formatted_results;
  NullLocalizer localizer;
  ProtoFormatter formatter(&localizer, &formatted_results);
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
  ASSERT_EQ(1, results.rule_results_size());
  ASSERT_EQ(1, results.rule_results(0).results_size());

  // Now instantiate an Engine with no Rules and attempt to format the
  // results. We expect this to fail since the Engine doesn't know
  // about the Rule in the Results structure.
  rules.clear();
  Engine engine2(&rules);
  engine2.Init();

  FormattedResults formatted_results;
  NullLocalizer localizer;
  ProtoFormatter formatter(&localizer, &formatted_results);
  ASSERT_FALSE(engine2.FormatResults(results, &formatter));
  ASSERT_EQ(0, formatted_results.rule_results_size());
}

TEST(EngineTest, NonFrozenInputFails) {
  PagespeedInput input;
  std::vector<Rule*> rules;
  rules.push_back(new TestRule());

  Engine engine(&rules);
  engine.Init();
  Results results;
#ifdef NDEBUG
  ASSERT_FALSE(engine.ComputeResults(input, &results));
  ASSERT_EQ(0, results.rule_results_size());
#else
  ASSERT_DEATH(engine.ComputeResults(input, &results),
               "Attempting to ComputeResults with non-frozen input.");
#endif
}

TEST(EngineTest, ResultIdAssignment) {
  PagespeedInput input;
  input.Freeze();

  std::vector<Rule*> rules;
  rules.push_back(new TestRule("rule1"));
  rules.push_back(new TestRule("rule2"));
  rules.push_back(new TestRule("rule3"));

  Engine engine(&rules);
  engine.Init();
  Results results;
  ASSERT_TRUE(engine.ComputeResults(input, &results));

  // Make sure the expected results were generated.
  ASSERT_EQ(3, results.rule_results_size());
  ASSERT_EQ(1, results.rule_results(0).results_size());
  ASSERT_EQ(1, results.rule_results(1).results_size());
  ASSERT_EQ(1, results.rule_results(2).results_size());

  // Make sure proper IDs were assigned.
  EXPECT_EQ(0, results.rule_results(0).results(0).id());
  EXPECT_EQ(1, results.rule_results(1).results(0).id());
  EXPECT_EQ(2, results.rule_results(2).results(0).id());
}

TEST(EngineTest, ComputeScoreOneExperimentalRule) {
  PagespeedInput input;
  input.Freeze();

  std::vector<Rule*> rules;
  rules.push_back(new TestExperimentalRule());

  Engine engine(&rules);
  engine.Init();
  Results results;
  ASSERT_TRUE(engine.ComputeResults(input, &results));
  ASSERT_EQ(100, results.score());
}

TEST(EngineTest, ComputeScoresWithExperimentalRule) {
  PagespeedInput input;
  input.Freeze();

  std::vector<Rule*> rules;
  rules.push_back(new TestRule());
  rules.push_back(new TestExperimentalRule());

  Engine engine(&rules);
  engine.Init();
  Results results;
  ASSERT_TRUE(engine.ComputeResults(input, &results));
  ASSERT_EQ(75, results.score());
}

TEST(EngineTest, FilterResults) {
  PagespeedInput input;
  input.Freeze();

  std::vector<Rule*> rules;
  rules.push_back(new TestRule());

  Engine engine(&rules);
  engine.Init();
  Results results;
  ASSERT_TRUE(engine.ComputeResults(input, &results));
  results.set_score(50);
  RuleResults* rule_results = results.mutable_rule_results(0);
  rule_results->set_rule_score(50);
  rule_results->set_rule_impact(5.0);

  Results filtered_results;
  NeverAcceptResultFilter filter;
  engine.FilterResults(results, filter, &filtered_results);

  ASSERT_EQ(100, filtered_results.score());
  ASSERT_EQ(1, filtered_results.rule_results_size());
  const RuleResults& filtered_rule_results =
      filtered_results.rule_results(0);
  ASSERT_EQ(0, filtered_rule_results.results_size());
  ASSERT_EQ(100, filtered_rule_results.rule_score());
  ASSERT_EQ(0, filtered_rule_results.rule_impact());

  // Try another filter.
  AlwaysAcceptResultFilter filter2;
  engine.FilterResults(results, filter2, &filtered_results);
  ASSERT_EQ(75, filtered_results.score());
  ASSERT_EQ(1, filtered_results.rule_results_size());
  const RuleResults& filtered_rule_results2 =
      filtered_results.rule_results(0);
  ASSERT_EQ(1, filtered_rule_results2.results_size());
  ASSERT_EQ(100, filtered_rule_results2.rule_score());
  ASSERT_EQ(0.25, filtered_rule_results2.rule_impact());
}


}  // namespace
