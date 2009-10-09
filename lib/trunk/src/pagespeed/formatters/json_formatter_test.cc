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

#include <iostream>

#include "pagespeed/formatters/json_formatter.h"
#include "testing/gtest/include/gtest/gtest.h"

using pagespeed::Argument;
using pagespeed::Formatter;
using pagespeed::formatters::JsonFormatter;

namespace {

TEST(JsonFormatterTest, BasicTest) {
  std::stringstream output;
  JsonFormatter formatter(&output);
  formatter.AddChild("foo");
  formatter.AddChild("bar");
  formatter.Done();
  std::string result = output.str();
  EXPECT_EQ("[\n{\"format\":[{\"type\":\"str\",\"value\":\"foo\"}]},\n"
            "{\"format\":[{\"type\":\"str\",\"value\":\"bar\"}]}]\n",
            result);
}

TEST(JsonFormatterTest, EscapeTest) {
  std::stringstream output;
  JsonFormatter formatter(&output);
  formatter.AddChild("\n\\\t\x12\f\"\r");
  formatter.Done();
  std::string result = output.str();
  EXPECT_EQ("[\n{\"format\":[{\"type\":\"str\","
            "\"value\":\"\\n\\\\\\t\\u0012\\f\\\"\\r\"}]}]\n",
            result);
}

TEST(JsonFormatterTest, TreeTest) {
  std::stringstream output;
  JsonFormatter formatter(&output);
  Formatter* level1 = formatter.AddChild("l1-1");
  Formatter* level2 = level1->AddChild("l2-1");
  Formatter* level3 = level2->AddChild("l3-1");
  level3->AddChild("l4-1");
  level3->AddChild("l4-2");
  level3 = level2->AddChild("l3-2");
  level3->AddChild("l4-3");
  level3->AddChild("l4-4");
  formatter.Done();
  std::string result = output.str();
  EXPECT_EQ("[\n{\"format\":[{\"type\":\"str\",\"value\":\"l1-1\"}],"
            "\"children\":[\n"
            "{\"format\":[{\"type\":\"str\",\"value\":\"l2-1\"}],"
            "\"children\":[\n"
            "{\"format\":[{\"type\":\"str\",\"value\":\"l3-1\"}],"
            "\"children\":[\n"
            "{\"format\":[{\"type\":\"str\",\"value\":\"l4-1\"}]},\n"
            "{\"format\":[{\"type\":\"str\",\"value\":\"l4-2\"}]}]},\n"
            "{\"format\":[{\"type\":\"str\",\"value\":\"l3-2\"}],"
            "\"children\":[\n"
            "{\"format\":[{\"type\":\"str\",\"value\":\"l4-3\"}]},\n"
            "{\"format\":[{\"type\":\"str\",\"value\":\"l4-4\"}]}]}]}]}]\n",
            result);
}

TEST(JsonFormatterTest, ArgumentTypesTest) {
  std::stringstream output;
  JsonFormatter formatter(&output);
  Argument bytes_arg(Argument::BYTES, 1536);
  Argument int_arg(Argument::INTEGER, 42);
  Argument string_arg(Argument::STRING, "test");
  Argument url_arg(Argument::URL, "http://test.com/");
  formatter.AddChild("$1", bytes_arg);
  formatter.AddChild("$1", int_arg);
  formatter.AddChild("$1", string_arg);
  formatter.AddChild("$1", url_arg);
  formatter.Done();
  std::string result = output.str();
  EXPECT_EQ("[\n{\"format\":[{\"type\":\"str\",\"value\":\"1.5KiB\"}]},\n"
            "{\"format\":[{\"type\":\"str\",\"value\":\"42\"}]},\n"
            "{\"format\":[{\"type\":\"str\",\"value\":\"test\"}]},\n"
            "{\"format\":[{\"type\":\"url\","
            "\"value\":\"http://test.com/\"}]}]\n",
            result);
}

TEST(JsonFormatterTest, ArgumentListTest) {
  std::stringstream output;
  JsonFormatter formatter(&output);
  Argument bytes_arg(Argument::BYTES, 1536);
  Argument int_arg(Argument::INTEGER, 42);
  Argument string_arg(Argument::STRING, "test");
  Argument url_arg(Argument::URL, "http://test.com/");
  formatter.AddChild("");
  formatter.AddChild("$1", bytes_arg);
  formatter.AddChild("$2 $1", bytes_arg, int_arg);
  formatter.AddChild("$1 $2 $3", bytes_arg, int_arg, string_arg);
  formatter.AddChild("$1 $4 $3 $2", bytes_arg, int_arg, string_arg, url_arg);
  formatter.Done();
  std::string result = output.str();
  EXPECT_EQ("[\n{\"format\":[]},\n"
            "{\"format\":[{\"type\":\"str\",\"value\":\"1.5KiB\"}]},\n"
            "{\"format\":[{\"type\":\"str\",\"value\":\"42 1.5KiB\"}]},\n"
            "{\"format\":[{\"type\":\"str\",\"value\":\"1.5KiB 42 test\"}]},\n"
            "{\"format\":[{\"type\":\"str\",\"value\":\"1.5KiB \"},"
            "{\"type\":\"url\",\"value\":\"http://test.com/\"},"
            "{\"type\":\"str\",\"value\":\" test 42\"}]}]\n",
            result);
}

}  // namespace
