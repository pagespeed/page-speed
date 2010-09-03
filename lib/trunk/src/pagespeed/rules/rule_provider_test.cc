// Copyright 2010 Google Inc. All Rights Reserved.
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

#include <vector>

#include "pagespeed/core/rule.h"
#include "pagespeed/rules/rule_provider.h"
#include "testing/gtest/include/gtest/gtest.h"

TEST(RuleProviderTest, AppendAllRules) {
  std::vector<pagespeed::Rule*> rules;
  pagespeed::rule_provider::AppendAllRules(false, &rules);
  ASSERT_FALSE(rules.empty());
}

TEST(RuleProviderTest, AppendCompatibleRulesNone) {
  std::vector<pagespeed::Rule*> rules;
  std::vector<std::string> incompatible_rule_names;
  pagespeed::rule_provider::AppendCompatibleRules(
      false,
      &rules,
      &incompatible_rule_names,
      pagespeed::InputCapabilities());
  // We expect that some rules only require "NONE" while others require more.
  ASSERT_FALSE(rules.empty());
  ASSERT_FALSE(incompatible_rule_names.empty());
}

TEST(RuleProviderTest, AppendCompatibleRulesAll) {
  std::vector<pagespeed::Rule*> rules;
  std::vector<std::string> incompatible_rule_names;
  pagespeed::rule_provider::AppendCompatibleRules(
      false,
      &rules,
      &incompatible_rule_names,
      pagespeed::InputCapabilities(pagespeed::InputCapabilities::ALL));
  ASSERT_TRUE(incompatible_rule_names.empty());

  std::vector<pagespeed::Rule*> all_rules;
  pagespeed::rule_provider::AppendAllRules(false, &all_rules);
  ASSERT_EQ(all_rules.size(), rules.size());
}
