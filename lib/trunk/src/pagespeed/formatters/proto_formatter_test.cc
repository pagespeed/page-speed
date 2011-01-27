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
// Author: aoates@google.com (Andrew Oates)

#include "pagespeed/formatters/proto_formatter.h"

#include <string>
#include <vector>

#include "base/logging.h"
#include "base/stl_util-inl.h"
#include "pagespeed/core/rule.h"
#include "pagespeed/l10n/user_facing_string.h"
#include "pagespeed/l10n/localizer.h"
#include "pagespeed/proto/pagespeed_output.pb.h"
#include "pagespeed/proto/pagespeed_proto_formatter.pb.h"
#include "testing/gtest/include/gtest/gtest.h"

using pagespeed::Argument;
using pagespeed::FormatArgument;
using pagespeed::FormatString;
using pagespeed::Formatter;
using pagespeed::FormatterParameters;
using pagespeed::UserFacingString;
using pagespeed::FormattedResults;
using pagespeed::FormattedRuleResults;
using pagespeed::formatters::ProtoFormatter;
using pagespeed::l10n::Localizer;
using pagespeed::l10n::NullLocalizer;

namespace {

#define _N(X) UserFacingString(X, true)

// Test localizer that outputs only the character '*'.
class TestLocalizer : public Localizer {
 public:
  TestLocalizer() : symbol_('*') {}

  const char* GetLocale() const { return "test"; }

  bool LocalizeString(const std::string& val, std::string* out) const {
    *out = std::string(val.length(), symbol_);
    return true;
  }

  bool LocalizeInt(int64 val, std::string* out) const {
    *out = "*";
    return true;
  }

  bool LocalizeUrl(const std::string& url, std::string* out) const {
    *out = std::string(url.length(), symbol_);
    return true;
  }

  bool LocalizeBytes(int64 bytes, std::string* out) const {
    *out = "**";
    return true;
  }

  bool LocalizeTimeDuration(int64 ms, std::string* out) const {
    *out = "***";
    return true;
  }

 private:
  char symbol_;

  DISALLOW_COPY_AND_ASSIGN(TestLocalizer);
};

class DummyTestRule : public pagespeed::Rule {
 public:
  explicit DummyTestRule(const UserFacingString& header)
      : pagespeed::Rule(pagespeed::InputCapabilities()),
        header_(header) {}

  virtual const char* name() const { return "DummyTestRule"; }
  virtual UserFacingString header() const { return header_; }
  virtual const char* documentation_url() const { return "doc.html"; }
  virtual bool AppendResults(const pagespeed::RuleInput& input,
                             pagespeed::ResultProvider* provider) {
    return true;
  }
  virtual void FormatResults(const pagespeed::ResultVector& results,
                             Formatter* formatter) {}
 private:
  UserFacingString header_;
};

TEST(ProtoFormatterTest, BasicTest) {
  FormattedResults results;
  NullLocalizer localizer;
  ProtoFormatter formatter(&localizer, &results);
  results.set_locale("en_US.UTF-8");

  DummyTestRule rule1(_N("rule1"));
  DummyTestRule rule2(_N("rule2"));

  Formatter* body = formatter.AddHeader(rule1, 100);
  Formatter* block = body->AddChild(_N("url block 1"));
  Formatter* url = block->AddChild(_N("URL 1"));
  url->AddChild(_N("URL 1, detail 1"));
  url->AddChild(_N("URL 1, detail 2"));
  url = block->AddChild(_N("URL 2"));
  url->AddChild(_N("URL 2, detail 1"));

  block = body->AddChild(_N("url block 2"));
  url = block->AddChild(_N("URL 3"));

  body = formatter.AddHeader(rule2, 50);
  block = body->AddChild(_N("url block 3"));
  url = block->AddChild(_N("URL 4"));

  ASSERT_TRUE(results.IsInitialized());

  ASSERT_EQ(2, results.rule_results_size());
  const FormattedRuleResults& r1 = results.rule_results(0);
  ASSERT_EQ("DummyTestRule", r1.rule_name());
  ASSERT_EQ(100, r1.rule_score());
  ASSERT_EQ("rule1", r1.localized_rule_name());
  ASSERT_EQ(2, r1.url_blocks_size());

  ASSERT_EQ("url block 1", r1.url_blocks(0).header().format());
  ASSERT_EQ(2, r1.url_blocks(0).urls_size());
  ASSERT_EQ("URL 1", r1.url_blocks(0).urls(0).result().format());
  ASSERT_EQ(2, r1.url_blocks(0).urls(0).details_size());
  ASSERT_EQ("URL 1, detail 1", r1.url_blocks(0).urls(0).details(0).format());
  ASSERT_EQ("URL 1, detail 2", r1.url_blocks(0).urls(0).details(1).format());
  ASSERT_EQ("URL 2", r1.url_blocks(0).urls(1).result().format());
  ASSERT_EQ(1, r1.url_blocks(0).urls(1).details_size());
  ASSERT_EQ("URL 2, detail 1", r1.url_blocks(0).urls(1).details(0).format());

  ASSERT_EQ("url block 2", r1.url_blocks(1).header().format());
  ASSERT_EQ(1, r1.url_blocks(1).urls_size());
  ASSERT_EQ("URL 3", r1.url_blocks(1).urls(0).result().format());
  ASSERT_EQ(0, r1.url_blocks(1).urls(0).details_size());

  const FormattedRuleResults& r2 = results.rule_results(1);
  ASSERT_EQ("DummyTestRule", r2.rule_name());
  ASSERT_EQ(50, r2.rule_score());
  ASSERT_EQ("rule2", r2.localized_rule_name());
  ASSERT_EQ(1, r2.url_blocks_size());

  ASSERT_EQ("url block 3", r2.url_blocks(0).header().format());
  ASSERT_EQ(1, r2.url_blocks(0).urls_size());
  ASSERT_EQ("URL 4", r2.url_blocks(0).urls(0).result().format());
  ASSERT_EQ(0, r2.url_blocks(0).urls(0).details_size());
}

TEST(ProtoFormatterTest, FormattingTest) {
  FormattedResults results;
  NullLocalizer localizer;
  ProtoFormatter formatter(&localizer, &results);
  results.set_locale("en_US.UTF-8");

  DummyTestRule rule1(_N("rule1"));

  Formatter* body = formatter.AddHeader(rule1, 100);
  Argument arg1(Argument::INTEGER, 50);
  Argument arg2(Argument::BYTES, 100);
  body->AddChild(_N("url block 1, $1 urls $2"), arg1, arg2);

  ASSERT_TRUE(results.IsInitialized());

  ASSERT_EQ(1, results.rule_results_size());
  const FormattedRuleResults& r1 = results.rule_results(0);
  ASSERT_EQ("DummyTestRule", r1.rule_name());
  ASSERT_EQ(100, r1.rule_score());
  ASSERT_EQ("rule1", r1.localized_rule_name());
  ASSERT_EQ(1, r1.url_blocks_size());

  const FormatString& header = r1.url_blocks(0).header();
  ASSERT_EQ("url block 1, $1 urls $2", header.format());
  ASSERT_EQ(2, header.args_size());
  ASSERT_EQ(FormatArgument::INT_LITERAL, header.args(0).type());
  ASSERT_EQ(50, header.args(0).int_value());
  ASSERT_EQ("50", header.args(0).localized_value());

  ASSERT_EQ(FormatArgument::BYTES, header.args(1).type());
  ASSERT_EQ(100, header.args(1).int_value());
  ASSERT_EQ("100", header.args(1).localized_value());
}

// Tests that the localizer is correctly invoked for all parameters.
TEST(ProtoFormatterTest, LocalizerTest) {
  FormattedResults results;
  TestLocalizer localizer;
  ProtoFormatter formatter(&localizer, &results);
  results.set_locale("en_US.UTF-8");

  DummyTestRule rule1(UserFacingString("rule1", true));
  DummyTestRule rule2(UserFacingString("rule2", false));

  Formatter* body = formatter.AddHeader(rule1, 100);
  std::vector<const Argument*> args;
  args.push_back(new Argument(Argument::URL, "http://www.google.com"));
  args.push_back(new Argument(Argument::STRING, "abcd"));
  args.push_back(new Argument(Argument::INTEGER, 100));
  args.push_back(new Argument(Argument::BYTES, 150));
  args.push_back(new Argument(Argument::DURATION, 200));
  STLElementDeleter<std::vector<const Argument*> > arg_deleter(&args);

  // Test a localized format string.
  UserFacingString format_str("text $1 $2 $3 $4 $5", true);
  FormatterParameters formatter_params(&format_str, &args);
  body->AddChild(formatter_params);

  // Test a non-localized format string.
  std::vector<const Argument*> args2;
  UserFacingString format_str2("not localized", false);
  FormatterParameters formatter_params2(&format_str2, &args2);
  body->AddChild(formatter_params2);

  // Test a non-localized rule header.
  formatter.AddHeader(rule2, 100);

  ASSERT_TRUE(results.IsInitialized());

  ASSERT_EQ(2, results.rule_results_size());
  const FormattedRuleResults& r1 = results.rule_results(0);
  EXPECT_EQ("DummyTestRule", r1.rule_name());
  EXPECT_EQ(100, r1.rule_score());
  EXPECT_EQ("*****", r1.localized_rule_name());
  ASSERT_EQ(2, r1.url_blocks_size());

  const FormatString& header = r1.url_blocks(0).header();
  EXPECT_EQ("*******************", header.format());
  ASSERT_EQ(5, header.args_size());

  EXPECT_EQ(FormatArgument::URL, header.args(0).type());
  EXPECT_FALSE(header.args(0).has_int_value());
  EXPECT_EQ("http://www.google.com", header.args(0).string_value());
  EXPECT_EQ("*********************", header.args(0).localized_value());

  // Test that string literals are *not* localized.
  EXPECT_EQ(FormatArgument::STRING_LITERAL, header.args(1).type());
  EXPECT_FALSE(header.args(1).has_int_value());
  EXPECT_EQ("abcd", header.args(1).string_value());
  EXPECT_EQ("abcd", header.args(1).localized_value());

  EXPECT_EQ(FormatArgument::INT_LITERAL, header.args(2).type());
  EXPECT_FALSE(header.args(2).has_string_value());
  EXPECT_EQ(100, header.args(2).int_value());
  EXPECT_EQ("*", header.args(2).localized_value());

  EXPECT_EQ(FormatArgument::BYTES, header.args(3).type());
  EXPECT_FALSE(header.args(3).has_string_value());
  EXPECT_EQ(150, header.args(3).int_value());
  EXPECT_EQ("**", header.args(3).localized_value());

  EXPECT_EQ(FormatArgument::DURATION, header.args(4).type());
  EXPECT_FALSE(header.args(4).has_string_value());
  EXPECT_EQ(200, header.args(4).int_value());
  EXPECT_EQ("***", header.args(4).localized_value());

  // Test non-localized format string.
  const FormatString& header2 = r1.url_blocks(1).header();
  EXPECT_EQ("not localized", header2.format());
  ASSERT_EQ(0, header2.args_size());

  // Test that string marked not localized isn't passed through the localizer.
  const FormattedRuleResults& r2 = results.rule_results(1);
  EXPECT_EQ("DummyTestRule", r2.rule_name());
  EXPECT_EQ(100, r2.rule_score());
  EXPECT_EQ("rule2", r2.localized_rule_name());
  ASSERT_EQ(0, r2.url_blocks_size());
}

} // namespace
