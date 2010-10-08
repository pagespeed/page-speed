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

#include "base/stl_util-inl.h"  // for STLDeleteContainerPointers
#include "pagespeed/proto/pagespeed_output.pb.h"
#include "pagespeed/formatters/proto_formatter.h"
#include "testing/gtest/include/gtest/gtest.h"

using pagespeed::Argument;
using pagespeed::FormatArgument;
using pagespeed::FormatterParameters;
using pagespeed::Formatter;
using pagespeed::formatters::ProtoFormatter;
using pagespeed::ResultText;

namespace {

class DummyTestRule : public pagespeed::Rule {
 public:
  explicit DummyTestRule(const char* header)
      : pagespeed::Rule(pagespeed::InputCapabilities()),
        header_(header) {}

  virtual const char* name() const { return "Dummy Test Rule"; }
  virtual const char* header() const { return header_; }
  virtual const char* documentation_url() const { return ""; }
  virtual bool AppendResults(const pagespeed::RuleInput& input,
                             pagespeed::ResultProvider* provider) {
    return true;
  }
  virtual void FormatResults(const pagespeed::ResultVector& results,
                             Formatter* formatter) {}
 private:
  const char* header_;
};

TEST(ProtoFormatterTest, BasicTest) {
  std::vector<ResultText*> results;
  ProtoFormatter formatter(&results);
  formatter.AddChild("foo");
  formatter.AddChild("bar");
  ASSERT_EQ(static_cast<size_t>(2), results.size());
  ResultText* first = results[0];
  EXPECT_EQ("foo", first->format());
  EXPECT_EQ(0, first->args_size());
  EXPECT_EQ(0, first->children_size());

  ResultText* second = results[1];
  EXPECT_EQ("bar", second->format());
  EXPECT_EQ(0, second->args_size());
  EXPECT_EQ(0, second->children_size());

  STLDeleteContainerPointers(results.begin(), results.end());
}

TEST(ProtoFormatterTest, OptimizedTest) {
  std::string format_str = "FooBar";
  FormatterParameters args(&format_str);
  std::string optimized = "<optimized result>";
  args.set_optimized_content(&optimized, "text/css");

  std::vector<ResultText*> results;
  ProtoFormatter formatter(&results);
  formatter.AddChild(args);
  formatter.Done();

  ASSERT_EQ(static_cast<size_t>(1), results.size());
  ResultText* first = results[0];
  EXPECT_EQ("FooBar", first->format());
  EXPECT_EQ(0, first->args_size());
  EXPECT_EQ(0, first->children_size());
  EXPECT_EQ("<optimized result>", first->optimized_content());

  STLDeleteContainerPointers(results.begin(), results.end());
}

TEST(ProtoFormatterTest, BasicHeaderTest) {
  std::vector<ResultText*> results;
  ProtoFormatter formatter(&results);
  DummyTestRule rule1("head");
  DummyTestRule rule2("head2");
  Formatter* child_formatter = formatter.AddHeader(rule1, 42);
  child_formatter->AddChild("foo");
  child_formatter->AddChild("bar");
  formatter.AddHeader(rule2, 23);
  formatter.Done();

  ASSERT_EQ(static_cast<size_t>(2), results.size());
  ResultText* first = results[0];
  EXPECT_EQ("head", first->format());
  EXPECT_EQ(0, first->args_size());
  ASSERT_EQ(2, first->children_size());
  EXPECT_EQ("foo", first->children(0).format());
  EXPECT_EQ("bar", first->children(1).format());

  ResultText* second = results[1];
  EXPECT_EQ("head2", second->format());
  EXPECT_EQ(0, second->args_size());
  EXPECT_EQ(0, second->children_size());

  STLDeleteContainerPointers(results.begin(), results.end());
}

TEST(ProtoFormatterTest, TreeTest) {
  std::vector<ResultText*> results;
  ProtoFormatter formatter(&results);
  Formatter* level1 = formatter.AddChild("l1-1");
  Formatter* level2 = level1->AddChild("l2-1");
  level2->AddChild("l3-1");
  level1->AddChild("l2-2");

  ASSERT_EQ(static_cast<size_t>(1), results.size());
  const ResultText& result = *results[0];
  EXPECT_EQ("l1-1", result.format());
  EXPECT_EQ(0, result.args_size());
  ASSERT_EQ(2, result.children_size());

  const ResultText& child_result1 = result.children(0);
  EXPECT_EQ("l2-1", child_result1.format());
  EXPECT_EQ(0, child_result1.args_size());
  ASSERT_EQ(1, child_result1.children_size());

  const ResultText& grandchild_result = child_result1.children(0);
  EXPECT_EQ("l3-1", grandchild_result.format());
  EXPECT_EQ(0, grandchild_result.args_size());
  EXPECT_EQ(0, grandchild_result.children_size());

  const ResultText& child_result2 = result.children(1);
  EXPECT_EQ("l2-2", child_result2.format());
  EXPECT_EQ(0, child_result2.args_size());
  EXPECT_EQ(0, child_result2.children_size());

  STLDeleteContainerPointers(results.begin(), results.end());
}

TEST(ProtoFormatterTest, ArgumentTypesTest) {
  std::vector<ResultText*> results;
  ProtoFormatter formatter(&results);

  Argument bytes_arg(Argument::BYTES, 23);
  Argument int_arg(Argument::INTEGER, 42);
  Argument string_arg(Argument::STRING, "test");
  Argument url_arg(Argument::URL, "http://test.com/");

  formatter.AddChild("$1", bytes_arg);
  formatter.AddChild("$1", int_arg);
  formatter.AddChild("$1", string_arg);
  formatter.AddChild("$1", url_arg);

  ASSERT_EQ(static_cast<size_t>(4), results.size());
  for (size_t idx = 0; idx < results.size(); idx++) {
    ResultText* result = results[idx];
    EXPECT_EQ("$1", result->format());
    ASSERT_EQ(1, result->args_size());
    EXPECT_EQ(0, result->children_size());
  }

  EXPECT_EQ(FormatArgument::BYTES, results[0]->args(0).type());
  EXPECT_EQ(23, results[0]->args(0).int_value());

  EXPECT_EQ(FormatArgument::INT_LITERAL, results[1]->args(0).type());
  EXPECT_EQ(42, results[1]->args(0).int_value());

  EXPECT_EQ(FormatArgument::STRING_LITERAL, results[2]->args(0).type());
  EXPECT_EQ("test", results[2]->args(0).string_value());

  EXPECT_EQ(FormatArgument::URL, results[3]->args(0).type());
  EXPECT_EQ("http://test.com/", results[3]->args(0).string_value());

  STLDeleteContainerPointers(results.begin(), results.end());
}

TEST(ProtoFormatterTest, ArgumentListTest) {
  std::vector<ResultText*> results;
  ProtoFormatter formatter(&results);

  Argument bytes_arg(Argument::BYTES, 23);
  Argument int_arg(Argument::INTEGER, 42);
  Argument string_arg(Argument::STRING, "test");
  Argument url_arg(Argument::URL, "http://test.com/");

  formatter.AddChild("");
  formatter.AddChild("$1", bytes_arg);
  formatter.AddChild("$1 $2", bytes_arg, int_arg);
  formatter.AddChild("$1 $2 $3", bytes_arg, int_arg, string_arg);
  formatter.AddChild("$1 $2 $3 $4", bytes_arg, int_arg, string_arg, url_arg);

  ASSERT_EQ(static_cast<size_t>(5), results.size());
  for (size_t idx = 0; idx < results.size(); idx++) {
    ResultText* result = results[idx];
    ASSERT_EQ(idx, static_cast<size_t>(result->args_size()));
    EXPECT_EQ(0, result->children_size());

    if (idx > 0) {
      EXPECT_EQ(FormatArgument::BYTES, result->args(0).type());
      EXPECT_EQ(23, result->args(0).int_value());
    }

    if (idx > 1) {
      EXPECT_EQ(FormatArgument::INT_LITERAL, result->args(1).type());
      EXPECT_EQ(42, result->args(1).int_value());
    }

    if (idx > 2) {
      EXPECT_EQ(FormatArgument::STRING_LITERAL, result->args(2).type());
      EXPECT_EQ("test", result->args(2).string_value());
    }

    if (idx > 3) {
      EXPECT_EQ(FormatArgument::URL, result->args(3).type());
      EXPECT_EQ("http://test.com/", result->args(3).string_value());
    }
  }

  STLDeleteContainerPointers(results.begin(), results.end());
}

}  // namespace
