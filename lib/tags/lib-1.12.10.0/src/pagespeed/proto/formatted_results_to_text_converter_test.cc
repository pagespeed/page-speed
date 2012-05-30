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

#include "base/scoped_ptr.h"
#include "base/values.h"
#include "pagespeed/proto/formatted_results_to_text_converter.h"
#include "pagespeed/proto/pagespeed_proto_formatter.pb.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

using pagespeed::FormatArgument;
using pagespeed::FormatString;
using pagespeed::FormattedResults;
using pagespeed::FormattedRuleResults;
using pagespeed::FormattedUrlResult;
using pagespeed::FormattedUrlBlockResults;
using pagespeed::proto::FormattedResultsToTextConverter;

TEST(FormattedResultsToTextConverterTest, NotInitialized) {
  FormattedResults results;
  std::string text;
  ASSERT_FALSE(FormattedResultsToTextConverter::Convert(results, &text));
}

TEST(FormattedResultsToTextConverterTest, Empty) {
  FormattedResults results;
  results.set_locale("test");

  std::string text;
  ASSERT_TRUE(FormattedResultsToTextConverter::Convert(results, &text));
  ASSERT_EQ("", text);
}

TEST(FormattedResultsToTextConverterTest, Basic) {
  FormattedResults results;
  results.set_locale("test");
  results.set_score(42);

  std::string text;
  ASSERT_TRUE(FormattedResultsToTextConverter::Convert(results, &text));
  ASSERT_EQ("**[42/100]**\n", text);
}

TEST(FormattedResultsToTextConverterTest, Full) {
  std::string expected;

  FormattedResults results;
  results.set_locale("test");

  FormattedRuleResults* rule_results1 = results.add_rule_results();
  rule_results1->set_rule_name("RuleName");
  rule_results1->set_localized_rule_name("LocalizedRuleName");
  rule_results1->set_rule_score(56);
  expected.append("_LocalizedRuleName_ (56/100)\n");

  FormattedUrlBlockResults* block = rule_results1->add_url_blocks();

  block->mutable_header()->set_format("Header format string.");
  expected.append("  Header format string.\n");

  FormattedUrlResult* result = block->add_urls();
  result->mutable_result()->set_format("http://www.example.com/");
  expected.append("    * http://www.example.com/\n");

  FormatString* format_string1 = result->add_details();

  // Add a few arguments to test argument serialization.
  FormatArgument* arg1 = format_string1->add_args();
  arg1->set_string_value("http://президент.рф/?<>");
  arg1->set_localized_value("http://президент.рф/?<>");
  arg1->set_type(FormatArgument::URL);

  FormatArgument* arg2 = format_string1->add_args();
  arg2->set_int_value(123);
  arg2->set_localized_value("123");
  arg2->set_type(FormatArgument::INT_LITERAL);

  format_string1->set_format("Here $1 is $2.");
  expected.append("      - Here http://президент.рф/?<> is 123.\n");

  // Add one more detail format string.
  FormatString* format_string2 = result->add_details();
  format_string2->set_format("Another one.");
  expected.append("      - Another one.\n");

  // Add one more URL so we test that the serializer correctly
  // serializes multiple entries.
  block->add_urls()->mutable_result()->set_format(
      "http://www.example.com/other");
  expected.append("    * http://www.example.com/other\n");

  // Add a second FormattedRuleResults
  FormattedRuleResults* rule_results2 = results.add_rule_results();
  rule_results2->set_rule_name("SecondRuleName");
  rule_results2->set_localized_rule_name("LocalizedSecondRuleName");
  expected.append("_LocalizedSecondRuleName_\n");

  results.set_score(12);
  expected.append("**[12/100]**\n");

  std::string text;
  ASSERT_TRUE(FormattedResultsToTextConverter::Convert(results, &text));
  ASSERT_EQ(expected, text);
}

}  // namespace
