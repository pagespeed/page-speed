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

#include "pagespeed/proto/results_to_json_converter.h"

#include "base/values.h"
#include "pagespeed/proto/pagespeed_output.pb.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

using pagespeed::InputInformation;
using pagespeed::Result;
using pagespeed::Results;
using pagespeed::RuleResults;
using pagespeed::Savings;
using pagespeed::Version;
using pagespeed::proto::ResultsToJsonConverter;

const char *kFullJson =
"{"
 "\"rule_results\":"
  "["
   "{"
    "\"results\":"
     "["
      "{"
       "\"resource_urls\":[\"http://www.foo.com/\",\"http://www.bar.com/\"],"
       "\"savings\":{\"dns_requests_saved\":1,\"response_bytes_saved\":500}"
      "}"
     "],"
    "\"rule_name\":\"Foo\","
    "\"rule_score\":56"
   "},"
   "{"
    "\"results\":"
     "["
      "{"
       "\"resource_urls\":[\"http://www.example.com/\"],"
       "\"savings\":{\"critical_path_length_saved\":3,\"requests_saved\":2}"
      "},"
      "{"
       "\"resource_urls\":[\"http://www.example.com/foo\"],"
       "\"savings\":{\"request_bytes_saved\":1}"
      "}"
     "],"
    "\"rule_name\":\"Bar\","
    "\"rule_score\":100"
   "}"
  "],"
 "\"score\":42,"
 "\"version\":{\"major\":1,\"minor\":9,\"official_release\":false}"
"}";

void PopulateBasicFields(Results* results) {
  // input_info is a required field, but none of its children are
  // required, so we simply instantiate it but don't populate any
  // fields.
  results->mutable_input_info();
  Version* version = results->mutable_version();
  version->set_major(1);
  version->set_minor(9);
  version->set_official_release(false);
}

TEST(ResultsToJsonConverterTest, NotInitialized) {
  Results results;
  std::string json;
  ASSERT_FALSE(ResultsToJsonConverter::Convert(results, &json));
}

TEST(ResultsToJsonConverterTest, Basic) {
  Results results;
  PopulateBasicFields(&results);

  std::string json;
  ASSERT_TRUE(ResultsToJsonConverter::Convert(results, &json));
  ASSERT_EQ(
      "{\"version\":{\"major\":1,\"minor\":9,\"official_release\":false}}",
      json);
}

TEST(ResultsToJsonConverterTest, Full) {
  Results results;
  PopulateBasicFields(&results);

  RuleResults* rule_results = results.add_rule_results();
  rule_results->set_rule_name("Foo");
  rule_results->set_rule_score(56);
  Result* result = rule_results->add_results();
  Savings* savings = result->mutable_savings();
  savings->set_dns_requests_saved(1);
  savings->set_response_bytes_saved(500);
  result->add_resource_urls("http://www.foo.com/");
  result->add_resource_urls("http://www.bar.com/");

  rule_results = results.add_rule_results();
  rule_results->set_rule_name("Bar");
  rule_results->set_rule_score(100);
  result = rule_results->add_results();
  savings = result->mutable_savings();
  savings->set_requests_saved(2);
  savings->set_critical_path_length_saved(3);
  result->add_resource_urls("http://www.example.com/");
  result = rule_results->add_results();
  savings = result->mutable_savings();
  savings->set_request_bytes_saved(1);
  result->add_resource_urls("http://www.example.com/foo");

  results.set_score(42);

  std::string json;
  ASSERT_TRUE(ResultsToJsonConverter::Convert(results, &json));
  ASSERT_EQ(kFullJson, json);

  Value* value = ResultsToJsonConverter::ConvertResults(results);
  ASSERT_NE(static_cast<Value*>(NULL), value);
  delete value;
}

TEST(ResultsToJsonConverterTest, ConvertVersion) {
  Version version;
  Value* value = ResultsToJsonConverter::ConvertVersion(version);
  ASSERT_EQ(NULL, value);

  version.set_major(1);
  version.set_minor(9);
  version.set_official_release(true);
  value = ResultsToJsonConverter::ConvertVersion(version);
  ASSERT_NE(static_cast<Value*>(NULL), value);
  delete value;
}

TEST(ResultsToJsonConverterTest, ConvertSavings) {
  Savings savings;
  Value* value = ResultsToJsonConverter::ConvertSavings(savings);
  ASSERT_NE(static_cast<Value*>(NULL), value);
  delete value;
}

TEST(ResultsToJsonConverterTest, ConvertResult) {
  Result result;
  Value* value = ResultsToJsonConverter::ConvertResult(result);
  ASSERT_NE(static_cast<Value*>(NULL), value);
  delete value;
}

TEST(ResultsToJsonConverterTest, ConvertRuleResult) {
  RuleResults rule_results;
  Value* value = ResultsToJsonConverter::ConvertRuleResult(rule_results);
  ASSERT_EQ(NULL, value);

  rule_results.set_rule_name("Foo");
  value = ResultsToJsonConverter::ConvertRuleResult(rule_results);
  ASSERT_NE(static_cast<Value*>(NULL), value);
  delete value;
}

}  // namespace
