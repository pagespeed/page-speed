// Copyright 2013 Google Inc. All Rights Reserved.
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

#include "pagespeed/core/formatter.h"
#include "pagespeed/l10n/l10n.h"
#include "pagespeed/testing/pagespeed_test.h"

namespace pagespeed {
  class ResultProvider;
  class RuleInput;
}

namespace {

class ExperimentalStringTestRule : public pagespeed::Rule {
 public:
  ExperimentalStringTestRule()
      : Rule(pagespeed::InputCapabilities()),
        is_experimental(false) {}

  virtual const char* name() const {
    return "ExperimentalStringTestRule";
  }

  virtual pagespeed::UserFacingString header() const {
    return not_localized("Rule to test experimental strings.");
  }

  virtual bool AppendResults(const pagespeed::RuleInput&,
                             pagespeed::ResultProvider*) {
    return true;
  }

  virtual void FormatResults(const pagespeed::ResultVector& results,
                             pagespeed::RuleFormatter* formatter) {
    formatter->SetSummaryLine(
        not_finalized("This string is not ready for translation."));
  }

  virtual bool IsExperimental() const {
    return is_experimental;
  }

  void SetExperimental(bool experimental) {
    is_experimental = experimental;
  }

 private:
  bool is_experimental;
};

class ExperimentalUserFacingStringTest
    : public pagespeed_testing::PagespeedRuleTest<ExperimentalStringTestRule> {
 protected:
};

TEST_F(ExperimentalUserFacingStringTest, ExperimentalRulePasses) {
  rule_->SetExperimental(true);
  Freeze();
  EXPECT_EQ("This string is not ready for translation.\n", FormatResults());
}

TEST_F(ExperimentalUserFacingStringTest, NonExperimentalRuleDfatal) {
  rule_->SetExperimental(false);
  Freeze();
  // EXPECT_DEBUG_DEATH uses a regex, so special characters need to be escaped.
  EXPECT_DEBUG_DEATH(
      FormatResults(),
     "Non-finalized translatable string used in non-experimental "
     "rule! Replace non_finalized\\(\\) with _\\(\\) so this user facing "
     "string can be localized\\.");
}

}  // namespace
