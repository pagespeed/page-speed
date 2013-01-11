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

#include "pagespeed/proto/formatted_results_to_json_converter.h"

#include "base/json/json_writer.h"
#include "base/memory/scoped_ptr.h"
#include "base/values.h"
#include "pagespeed/proto/pagespeed_proto_formatter.pb.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

using pagespeed::FormatArgument;
using pagespeed::FormatString;
using pagespeed::FormattedResults;
using pagespeed::FormattedRuleResults;
using pagespeed::FormattedUrlResult;
using pagespeed::FormattedUrlBlockResults;
using pagespeed::proto::FormattedResultsToJsonConverter;

TEST(FormattedResultsToJsonConverterTest, NotInitialized) {
  FormattedResults results;
  std::string json;
  ASSERT_FALSE(FormattedResultsToJsonConverter::Convert(results, &json));
}

TEST(FormattedResultsToJsonConverterTest, Basic) {
  FormattedResults results;
  results.set_locale("test");

  std::string json;
  ASSERT_TRUE(FormattedResultsToJsonConverter::Convert(results, &json));
  ASSERT_EQ("{\"locale\":\"test\"}", json);
}

TEST(FormattedResultsToJsonConverterTest, Full) {
  std::string expected;

  FormattedResults results;
  expected.append("{");

  results.set_locale("test");
  expected.append("\"locale\":\"test\",");

  FormattedRuleResults* rule_results = results.add_rule_results();
  expected.append("\"rule_results\":[{");

  rule_results->set_localized_rule_name("LocalizedRuleName");
  expected.append("\"localized_rule_name\":\"LocalizedRuleName\",");

  rule_results->set_rule_name("RuleName");
  expected.append("\"rule_name\":\"RuleName\",");

  rule_results->set_rule_score(56);
  expected.append("\"rule_score\":56,");

  FormattedUrlBlockResults* block = rule_results->add_url_blocks();
  expected.append("\"url_blocks\":[{");

  block->set_associated_result_id(17);
  expected.append("\"associated_result_id\":17,");

  // Add one more URL block so we test that the serializer correctly
  // serializes multiple entries.
  block->mutable_header()->set_format("Header format string.");
  expected.append("\"header\":{\"format\":\"Header format string.\"},");

  FormattedUrlResult* result = block->add_urls();
  expected.append("\"urls\":[{");

  result->set_associated_result_id(42);
  expected.append("\"associated_result_id\":42,");

  FormatString* format_string = result->add_details();
  expected.append("\"details\":[{");

  // Add a few arguments to test argument serialization.
  FormatArgument* arg = format_string->add_args();
  expected.append("\"args\":[{");

  arg->set_localized_value("http://президент.рф/?<>");
  expected.append(
      "\"localized_value\":"
      "\"http://\\u043F\\u0440\\u0435\\u0437\\u0438\\u0434\\u0435\\u043D"
      "\\u0442.\\u0440\\u0444/\?\\u003C\\u003E\",");

  arg->set_string_value("http://президент.рф/?<>");
  expected.append(
      "\"string_value\":"
      "\"http://\\u043F\\u0440\\u0435\\u0437\\u0438\\u0434\\u0435\\u043D"
      "\\u0442.\\u0440\\u0444/\?\\u003C\\u003E\",");

  arg->set_type(FormatArgument::URL);
  expected.append("\"type\":\"URL\"");

  arg = format_string->add_args();
  expected.append("},{");

  arg->set_int_value(123);
  expected.append("\"int_value\":123,");

  arg->set_localized_value("123");
  expected.append("\"localized_value\":\"123\",");

  arg->set_type(FormatArgument::INT_LITERAL);
  expected.append("\"type\":\"INT_LITERAL\"");

  expected.append("}],");

  format_string->set_format("Here $1 is $2.");
  expected.append("\"format\":\"Here $1 is $2.\"");

  // Add one more detail format string.
  format_string = result->add_details();
  expected.append("},{");

  format_string->set_format("Another one.");
  expected.append("\"format\":\"Another one.\"");

  result->mutable_result()->set_format("http://www.example.com/");
  expected.append("}],");
  expected.append("\"result\":{\"format\":\"http://www.example.com/\"}");

  expected.append("},{");

  // Add one more URL so we test that the serializer correctly
  // serializes multiple entries.
  block->add_urls()->mutable_result()->set_format(
      "http://www.example.com/other");

  expected.append("\"result\":");
  expected.append("{\"format\":\"http://www.example.com/other\"}");
  expected.append("}]},");

  rule_results->add_url_blocks()->mutable_header()->set_format("One more.");
  expected.append("{\"header\":{\"format\":\"One more.\"}}");
  expected.append("]},");

  // Add a second FormattedRuleResults
  rule_results = results.add_rule_results();
  expected.append("{");

  rule_results->set_localized_rule_name("LocalizedSecondRuleName");
  expected.append("\"localized_rule_name\":\"LocalizedSecondRuleName\",");

  rule_results->set_rule_name("SecondRuleName");
  expected.append("\"rule_name\":\"SecondRuleName\"");
  expected.append("}],");

  results.set_score(12);
  expected.append("\"score\":12");
  expected.append("}");

  std::string json;
  ASSERT_TRUE(FormattedResultsToJsonConverter::Convert(results, &json));
  ASSERT_EQ(expected, json);

  scoped_ptr<Value> value(
      FormattedResultsToJsonConverter::ConvertFormattedResults(results));
  ASSERT_NE(static_cast<Value*>(NULL), value);
}

TEST(FormattedResultsToJsonConverterTest, ConvertFormatArgumentType) {
  EXPECT_STREQ("INVALID",
               FormattedResultsToJsonConverter::ConvertFormatArgumentType(0));

  EXPECT_STREQ("URL",
               FormattedResultsToJsonConverter::ConvertFormatArgumentType(
                   FormatArgument::URL));

  EXPECT_STREQ("STRING_LITERAL",
               FormattedResultsToJsonConverter::ConvertFormatArgumentType(
                   FormatArgument::STRING_LITERAL));

  EXPECT_STREQ("INT_LITERAL",
               FormattedResultsToJsonConverter::ConvertFormatArgumentType(
                   FormatArgument::INT_LITERAL));

  EXPECT_STREQ("BYTES",
               FormattedResultsToJsonConverter::ConvertFormatArgumentType(
                   FormatArgument::BYTES));

  EXPECT_STREQ("DURATION",
               FormattedResultsToJsonConverter::ConvertFormatArgumentType(
                   FormatArgument::DURATION));

  EXPECT_STREQ("VERBATIM_STRING",
               FormattedResultsToJsonConverter::ConvertFormatArgumentType(
                   FormatArgument::VERBATIM_STRING));

  EXPECT_STREQ("PERCENTAGE",
               FormattedResultsToJsonConverter::ConvertFormatArgumentType(
                   FormatArgument::PERCENTAGE));

  EXPECT_STREQ("INVALID",
               FormattedResultsToJsonConverter::ConvertFormatArgumentType(
                   FormatArgument::PERCENTAGE + 1));
}

TEST(FormattedResultsToJsonConverterTest, ConvertFormatArgument) {
  FormatArgument arg;
  scoped_ptr<Value> value(
      FormattedResultsToJsonConverter::ConvertFormatArgument(arg));
  ASSERT_EQ(NULL, value.get());

  arg.set_type(FormatArgument::INT_LITERAL);
  value.reset(FormattedResultsToJsonConverter::ConvertFormatArgument(arg));
  ASSERT_EQ(NULL, value.get());

  arg.set_localized_value("1");
  arg.set_int_value(1);
  arg.set_string_value("hello");

  value.reset(FormattedResultsToJsonConverter::ConvertFormatArgument(arg));
  ASSERT_NE(static_cast<Value*>(NULL), value.get());

  std::string out;
  base::JSONWriter::Write(value.get(), false, &out);
  ASSERT_EQ("{\"int_value\":1,\"localized_value\":\"1\","
            "\"string_value\":\"hello\",\"type\":\"INT_LITERAL\"}", out);
}

TEST(FormattedResultsToJsonConverterTest, ConvertFormatString) {
  FormatString format_str;
  scoped_ptr<Value> value(
      FormattedResultsToJsonConverter::ConvertFormatString(format_str));
  ASSERT_EQ(NULL, value.get());

  format_str.set_format("This is a format string.");
  value.reset(FormattedResultsToJsonConverter::ConvertFormatString(format_str));
  ASSERT_NE(static_cast<Value*>(NULL), value.get());

  std::string out;
  base::JSONWriter::Write(value.get(), false, &out);
  ASSERT_EQ("{\"format\":\"This is a format string.\"}", out);
}

TEST(FormattedResultsToJsonConverterTest, ConvertFormattedUrlResult) {
  FormattedUrlResult result;
  scoped_ptr<Value> value(
      FormattedResultsToJsonConverter::ConvertFormattedUrlResult(result));
  ASSERT_EQ(NULL, value.get());
}

TEST(FormattedResultsToJsonConverterTest, ConvertFormattedUrlBlockResults) {
  FormattedUrlBlockResults result;
  scoped_ptr<Value> value(
      FormattedResultsToJsonConverter::ConvertFormattedUrlBlockResults(result));
  ASSERT_NE(static_cast<Value*>(NULL), value.get());

  std::string out;
  base::JSONWriter::Write(value.get(), false, &out);
  ASSERT_EQ("{}", out);
}

TEST(FormattedResultsToJsonConverterTest, ConvertFormattedRuleResults) {
  FormattedRuleResults result;
  scoped_ptr<Value> value(
      FormattedResultsToJsonConverter::ConvertFormattedRuleResults(result));
  ASSERT_EQ(NULL, value.get());
}

TEST(FormattedResultsToJsonConverterTest, ConvertFormattedResults) {
  FormattedResults result;
  scoped_ptr<Value> value(
      FormattedResultsToJsonConverter::ConvertFormattedResults(result));
  ASSERT_EQ(NULL, value.get());
}

TEST(FormattedResultsToJsonConverterTest, InvalidUtf8) {
  // The bytes 0xc2 and 0xc3 indicate the start of a 2-character UTF8
  // character. However, when the following character is ' ' (0x20),
  // it is not a valid UTF8 character. We expect the 0xc2 and 0xc3
  // bytes to be converted to the unicode replacement character U+FFFD
  // when formatting the UTF8 sequence. We include \xc2\xa1 "¡" in the
  // sequence to verify that we do still process valid UTF8
  // characters.
  const char* kInvalidUtf8 = "hello\xc2 \xc2\xa1\xc3 hello";

  FormatArgument arg;
  arg.set_type(FormatArgument::STRING_LITERAL);
  arg.set_localized_value("localized foo");
  arg.set_string_value(kInvalidUtf8);

#ifndef NDEBUG
  ASSERT_DEATH(
      FormattedResultsToJsonConverter::ConvertFormatArgument(arg),
      "");
#else
  scoped_ptr<Value> value(
      FormattedResultsToJsonConverter::ConvertFormatArgument(arg));
  ASSERT_NE(static_cast<Value*>(NULL), value.get());

  std::string out;
  base::JSONWriter::Write(value.get(), false, &out);
  ASSERT_EQ("{\"localized_value\":\"localized foo\","
            "\"string_value\":"
            "\"hello\\uFFFD \\u00A1\\uFFFD hello\",\"type\":\"STRING_LITERAL\"}", out);
#endif
}


}  // namespace
