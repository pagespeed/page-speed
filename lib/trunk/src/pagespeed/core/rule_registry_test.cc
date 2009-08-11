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

#include <vector>

#include "base/scoped_ptr.h"
#include "base/stl_util-inl.h"
#include "pagespeed/core/pagespeed_options.pb.h"
#include "pagespeed/core/rule.h"
#include "pagespeed/core/rule_registry.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace pagespeed {
class PagespeedInput;
class Results;
}  // namespace pagespeed

using pagespeed::Options;
using pagespeed::PagespeedInput;
using pagespeed::Results;
using pagespeed::Rule;
using pagespeed::RuleRegistry;

namespace {

class TestRule : public Rule {
 public:
  TestRule() {}
  virtual ~TestRule() {}

  virtual bool AppendResults(const PagespeedInput& input, Results* results) {
    return true;
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(TestRule);
};

// Register the test rule. Use the ENABLE_GZIP identifier for the
// purposes of testing.
REGISTER_PAGESPEED_RULE(TestRule);

class RuleRegistryTest : public testing::Test {
 public:
  static void SetUpTestCase() {
    RuleRegistry::Freeze();
  }
};

TEST_F(RuleRegistryTest, RegisteredRuleTest) {
  Options options;
  options.add_rule_names("TestRule");

  std::vector<Rule*> rule_instances;
  RuleRegistry::CreateRuleInstances(options, &rule_instances);
  ASSERT_EQ(1, rule_instances.size());
  ASSERT_NE(static_cast<Rule*>(NULL), rule_instances[0]);
  STLDeleteContainerPointers(rule_instances.begin(), rule_instances.end());
}

TEST_F(RuleRegistryTest, UnregisteredRuleTest) {
  Options options;
  options.add_rule_names("UnknownRule");

  std::vector<Rule*> rule_instances;
  ASSERT_DEATH(RuleRegistry::CreateRuleInstances(options, &rule_instances),
               "No handler for \"UnknownRule\"");
}

}  // namespace
