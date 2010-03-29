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

#include "pagespeed/core/rule.h"
#include "pagespeed/core/serializer.h"
#include "pagespeed/formatters/json_formatter.h"
#include "testing/gtest/include/gtest/gtest.h"

using pagespeed::Argument;
using pagespeed::Formatter;
using pagespeed::FormatterParameters;
using pagespeed::formatters::JsonFormatter;

namespace {

class DummyTestRule : public pagespeed::Rule {
 public:
  explicit DummyTestRule(const char* header) : header_(header) {}

  virtual const char* name() const { return "DummyTestRule"; }
  virtual const char* header() const { return header_; }
  virtual const char* documentation_url() const { return "doc.html"; }
  virtual bool AppendResults(const pagespeed::PagespeedInput& input,
                             pagespeed::ResultProvider* provider) {
    return true;
  }
  virtual void FormatResults(const pagespeed::ResultVector& results,
                             Formatter* formatter) {}
 private:
  const char* header_;
};

class DummyTestSerializer : public pagespeed::Serializer {
 public:
  // Serializer interface
  virtual std::string SerializeToFile(const std::string& content_url,
                                      const std::string& mime_type,
                                      const std::string& body) {
    return ("serialize url: " + content_url + " mime: " + mime_type +
            " body: " + body);
  }
};

TEST(JsonFormatterTest, BasicTest) {
  std::stringstream output;
  JsonFormatter formatter(&output, NULL);
  formatter.AddChild("foo");
  formatter.AddChild("bar");
  formatter.Done();
  std::string result = output.str();
  EXPECT_EQ("[\n{\"format\":[{\"type\":\"str\",\"value\":\"foo\"}]},\n"
            "{\"format\":[{\"type\":\"str\",\"value\":\"bar\"}]}]\n",
            result);
}

TEST(JsonFormatterTest, BasicHeaderTest) {
  std::stringstream output;
  JsonFormatter formatter(&output, NULL);
  DummyTestRule rule1("head");
  DummyTestRule rule2("head2");
  Formatter* child_formatter = formatter.AddHeader(rule1, 42);
  child_formatter->AddChild("foo");
  child_formatter->AddChild("bar");
  formatter.AddHeader(rule2, 23);
  formatter.Done();
  std::string result = output.str();
  EXPECT_EQ("[\n"
            "{\"format\":[{\"type\":\"str\",\"value\":\"head\"}],"
            "\"name\":\"DummyTestRule\","
            "\"score\":42,"
            "\"url\":\"doc.html\","
            "\"children\":[\n"
            "{\"format\":[{\"type\":\"str\",\"value\":\"foo\"}]},\n"
            "{\"format\":[{\"type\":\"str\",\"value\":\"bar\"}]}]"
            "},\n"
            "{\"format\":[{\"type\":\"str\",\"value\":\"head2\"}],"
            "\"name\":\"DummyTestRule\","
            "\"score\":23,"
            "\"url\":\"doc.html\"}"
            "]\n",
            result);
}

TEST(JsonFormatterTest, EscapeTest) {
  std::stringstream output;
  JsonFormatter formatter(&output, NULL);
  formatter.AddChild("\n\\\t\x12\f\"\r<>");
  formatter.Done();
  std::string result = output.str();
  EXPECT_EQ("[\n{\"format\":[{\"type\":\"str\","
            "\"value\":\"\\n\\\\\\t\\u0012\\f\\\"\\r\\x3c\\x3e\"}]}]\n",
            result);
}

TEST(JsonFormatterTest, UrlEscapeTest) {
  std::stringstream output;
  JsonFormatter formatter(&output, NULL);
  Argument url_arg(Argument::URL, "http://a.com/\n\\\t\x12\f\"\r<>");
  formatter.AddChild("url: $1", url_arg);
  formatter.Done();
  std::string result = output.str();
  EXPECT_EQ(
      "[\n{\"format\":[{\"type\":\"str\","
      "\"value\":\"url: \"},"
      "{\"type\":\"url\","
      "\"value\":\"http://a.com/\\n\\\\\\t\\u0012\\f\\\"\\r\\x3c\\x3e\"}]}]\n",
      result);

}

TEST(JsonFormatterTest, TreeTest) {
  std::stringstream output;
  JsonFormatter formatter(&output, NULL);
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
  JsonFormatter formatter(&output, NULL);
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

TEST(JsonFormatterTest, OptimizedTest) {
  std::string format_str = "FooBar $1";
  Argument url_arg(Argument::URL, "http://test.com/");
  std::vector<const Argument*> arguments;
  arguments.push_back(&url_arg);
  FormatterParameters args(&format_str, &arguments);
  std::string optimized = "<optimized result>";
  args.set_optimized_content(&optimized, "text/css");

  std::stringstream output;
  DummyTestSerializer serializer;
  JsonFormatter formatter(&output, &serializer);
  formatter.AddChild(args);
  formatter.Done();

  std::string result = output.str();
  EXPECT_EQ(
      "[\n{\"format\":[{\"type\":\"str\","
      "\"value\":\"FooBar \"},"
      "{\"type\":\"url\",\"value\":\"http://test.com/\"},"
      "{\"type\":\"str\",\"value\":\"  See \"},"
      "{\"type\":\"url\","
      "\"value\":"
      "\"serialize url: http://test.com/ mime: text/css "
      "body: \\x3coptimized result\\x3e\","
      "\"alt\":\"optimized version\"},"
      "{\"type\":\"str\",\"value\":\".\"}"
      "]}]\n",
      result);
}

TEST(JsonFormatterTest, OptimizedTestNoUrl) {
  std::string format_str = "FooBar";
  FormatterParameters args(&format_str);
  std::string optimized = "<optimized result>";
  args.set_optimized_content(&optimized, "text/css");

  std::stringstream output;
  DummyTestSerializer serializer;
  JsonFormatter formatter(&output, &serializer);
  formatter.AddChild(args);
  formatter.Done();

  std::string result = output.str();
  EXPECT_EQ("[\n{\"format\":[{\"type\":\"str\","
            "\"value\":\"FooBar\"},"
            "{\"type\":\"str\",\"value\":\"  See \"},"
            "{\"type\":\"url\","
            "\"value\":\"serialize url:  mime: text/css "
            "body: \\x3coptimized result\\x3e\","
            "\"alt\":\"optimized version\"},"
            "{\"type\":\"str\",\"value\":\".\"}"
            "]}]\n",
            result);
}

TEST(JsonFormatterTest, ArgumentListTest) {
  std::stringstream output;
  JsonFormatter formatter(&output, NULL);
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

TEST(JsonFormatterTest, FormatBytesTest) {
  std::stringstream output;
  JsonFormatter formatter(&output, NULL);
  Argument bytes1(Argument::BYTES, 617);
  Argument bytes2(Argument::BYTES, 1024);
  Argument bytes3(Argument::BYTES, 1 << 21);
  formatter.AddChild("$1 | $2 | $3", bytes1, bytes2, bytes3);
  formatter.Done();
  std::string result = output.str();
  EXPECT_EQ("[\n{\"format\":[{\"type\":\"str\",\"value\":\"617B | 1.0KiB |"
            " 2.0MiB\"}]}]\n",
            result);
}

}  // namespace
