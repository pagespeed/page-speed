// Copyright 2010 Google Inc. All Rights Reserved.
// Author: aoates@google.com (Andrew Oates)

#include "pagespeed/formatters/proto_formatter.h"

#include <string>
#include <vector>

#include "base/logging.h"
#include "base/stl_util-inl.h"
#include "pagespeed/core/rule.h"
#include "pagespeed/l10n/localizable_string.h"
#include "pagespeed/l10n/localizer.h"
#include "pagespeed/proto/pagespeed_output.pb.h"
#include "pagespeed/proto/pagespeed_proto_formatter.pb.h"
#include "testing/gtest/include/gtest/gtest.h"

using pagespeed::Argument;
using pagespeed::FormatArgument;
using pagespeed::FormatString;
using pagespeed::Formatter;
using pagespeed::FormatterParameters;
using pagespeed::LocalizableString;
using pagespeed::FormattedResults;
using pagespeed::FormattedRuleResults;
using pagespeed::formatters::ProtoFormatter;
using pagespeed::l10n::Localizer;
using pagespeed::l10n::NullLocalizer;

namespace {

#define _N(X) LocalizableString(X)

// Test localizer that outputs only the character '*'
class TestLocalizer : public Localizer {
 public:
  TestLocalizer() : symbol_('*') {}

  bool SetLocale(const std::string& locale) { return false; }

  std::string LocalizeString(const std::string& val) const {
    return std::string(val.length(), symbol_);
  }

  std::string LocalizeInt(int64 val) const { return "*"; }

  std::string LocalizeUrl(const std::string& url) const {
    return std::string(url.length(), symbol_);
  }

  std::string LocalizeBytes(int64 bytes) const { return "**"; }

  std::string LocalizeTimeDuration(int64 ms) const { return "***"; }

 private:
  char symbol_;

  DISALLOW_COPY_AND_ASSIGN(TestLocalizer);
};

class DummyTestRule : public pagespeed::Rule {
 public:
  explicit DummyTestRule(const LocalizableString& header)
      : pagespeed::Rule(pagespeed::InputCapabilities()),
        header_(header) {}

  virtual const char* name() const { return "DummyTestRule"; }
  virtual LocalizableString header() const { return header_; }
  virtual const char* documentation_url() const { return "doc.html"; }
  virtual bool AppendResults(const pagespeed::RuleInput& input,
                             pagespeed::ResultProvider* provider) {
    return true;
  }
  virtual void FormatResults(const pagespeed::ResultVector& results,
                             Formatter* formatter) {}
 private:
  LocalizableString header_;
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
  ASSERT_EQ("DummyTestRule", r1.rule());
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
  ASSERT_EQ("DummyTestRule", r2.rule());
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
  ASSERT_EQ("DummyTestRule", r1.rule());
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

// Tests that the localizer is correctly invoked for all parameters
TEST(ProtoFormatterTest, LocalizerTest) {
  FormattedResults results;
  TestLocalizer localizer;
  ProtoFormatter formatter(&localizer, &results);
  results.set_locale("en_US.UTF-8");

  DummyTestRule rule1(_N("rule1"));

  Formatter* body = formatter.AddHeader(rule1, 100);
  std::vector<const Argument*> args;
  args.push_back(new Argument(Argument::URL, "http://www.google.com"));
  args.push_back(new Argument(Argument::STRING, "abcd"));
  args.push_back(new Argument(Argument::INTEGER, 100));
  args.push_back(new Argument(Argument::BYTES, 150));
  args.push_back(new Argument(Argument::DURATION, 200));
  STLElementDeleter<std::vector<const Argument*> > arg_deleter(&args);

  LocalizableString format_str = _N("text $1 $2 $3 $4 $5");
  FormatterParameters formatter_params(&format_str, &args);
  body->AddChild(formatter_params);

  ASSERT_TRUE(results.IsInitialized());

  ASSERT_EQ(1, results.rule_results_size());
  const FormattedRuleResults& r1 = results.rule_results(0);
  ASSERT_EQ("DummyTestRule", r1.rule());
  ASSERT_EQ("*****", r1.localized_rule_name());
  ASSERT_EQ(1, r1.url_blocks_size());

  const FormatString& header = r1.url_blocks(0).header();
  ASSERT_EQ("*******************", header.format());
  ASSERT_EQ(5, header.args_size());

  ASSERT_EQ(FormatArgument::URL, header.args(0).type());
  ASSERT_FALSE(header.args(0).has_int_value());
  ASSERT_EQ("http://www.google.com", header.args(0).string_value());
  ASSERT_EQ("*********************", header.args(0).localized_value());

  ASSERT_EQ(FormatArgument::STRING_LITERAL, header.args(1).type());
  ASSERT_FALSE(header.args(1).has_int_value());
  ASSERT_EQ("abcd", header.args(1).string_value());
  ASSERT_EQ("****", header.args(1).localized_value());

  ASSERT_EQ(FormatArgument::INT_LITERAL, header.args(2).type());
  ASSERT_FALSE(header.args(2).has_string_value());
  ASSERT_EQ(100, header.args(2).int_value());
  ASSERT_EQ("*", header.args(2).localized_value());

  ASSERT_EQ(FormatArgument::BYTES, header.args(3).type());
  ASSERT_FALSE(header.args(3).has_string_value());
  ASSERT_EQ(150, header.args(3).int_value());
  ASSERT_EQ("**", header.args(3).localized_value());

  ASSERT_EQ(FormatArgument::DURATION, header.args(4).type());
  ASSERT_FALSE(header.args(4).has_string_value());
  ASSERT_EQ(200, header.args(4).int_value());
  ASSERT_EQ("***", header.args(4).localized_value());
}

} // namespace
