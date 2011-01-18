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
#include "pagespeed/l10n/l10n.h"
#include "pagespeed/formatters/json_formatter.h"
#include "testing/gtest/include/gtest/gtest.h"

using pagespeed::Argument;
using pagespeed::Formatter;
using pagespeed::FormatterParameters;
using pagespeed::formatters::JsonFormatter;
using pagespeed::UserFacingString;

namespace {

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
  formatter.AddChild(not_localized("foo"));
  formatter.AddChild(not_localized("bar"));
  formatter.Done();
  std::string result = output.str();
  EXPECT_EQ("[\n{\"format\":[{\"type\":\"str\",\"value\":\"foo\"}]},\n"
            "{\"format\":[{\"type\":\"str\",\"value\":\"bar\"}]}]\n",
            result);
}

TEST(JsonFormatterTest, BasicHeaderTest) {
  std::stringstream output;
  JsonFormatter formatter(&output, NULL);
  DummyTestRule rule1(not_localized("head"));
  DummyTestRule rule2(not_localized("head2"));
  Formatter* child_formatter = formatter.AddHeader(rule1, 42);
  child_formatter->AddChild(not_localized("foo"));
  child_formatter->AddChild(not_localized("bar"));
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
  formatter.AddChild(not_localized("\n\\\t\x12\f\"\r<>"));
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
  formatter.AddChild(not_localized("url: $1"), url_arg);
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
  Formatter* level1 = formatter.AddChild(not_localized("l1-1"));
  Formatter* level2 = level1->AddChild(not_localized("l2-1"));
  Formatter* level3 = level2->AddChild(not_localized("l3-1"));
  level3->AddChild(not_localized("l4-1"));
  level3->AddChild(not_localized("l4-2"));
  level3 = level2->AddChild(not_localized("l3-2"));
  level3->AddChild(not_localized("l4-3"));
  level3->AddChild(not_localized("l4-4"));
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
  formatter.AddChild(not_localized("$1"), bytes_arg);
  formatter.AddChild(not_localized("$1"), int_arg);
  formatter.AddChild(not_localized("$1"), string_arg);
  formatter.AddChild(not_localized("$1"), url_arg);
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
  UserFacingString format_str = not_localized("FooBar $1");
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
  UserFacingString format_str = not_localized("FooBar");
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
  formatter.AddChild(not_localized(""));
  formatter.AddChild(not_localized("$1"), bytes_arg);
  formatter.AddChild(not_localized("$2 $1"), bytes_arg, int_arg);
  formatter.AddChild(not_localized("$1 $2 $3"), bytes_arg, int_arg, string_arg);
  formatter.AddChild(not_localized("$1 $4 $3 $2"),
                     bytes_arg, int_arg, string_arg, url_arg);
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
  formatter.AddChild(not_localized("$1 | $2 | $3"), bytes1, bytes2, bytes3);
  formatter.Done();
  std::string result = output.str();
  EXPECT_EQ("[\n{\"format\":[{\"type\":\"str\",\"value\":\"617B | 1.0KiB |"
            " 2.0MiB\"}]}]\n",
            result);
}

TEST(JsonFormatterTest, FormatUtf8) {
  std::stringstream output;
  JsonFormatter formatter(&output, NULL);
  Argument url(Argument::URL, "http://президент.рф/?<>");
  formatter.AddChild(not_localized("$1"), url);
  formatter.Done();
  std::string result = output.str();
  EXPECT_EQ("[\n{\"format\":[{\"type\":\"url\",\"value\":"
            "\"http://президент.рф/?\\x3c\\x3e\""
            "}]}]\n",
            result);
}

TEST(JsonFormatterTest, FormatInvalidUtf8) {
  // The bytes 0xc2 and 0xc3 indicate the start of a 2-character UTF8
  // character. However, when the following character is ' ' (0x20),
  // it is not a valid UTF8 character. We expect the 0xc2 and 0xc3
  // bytes to be skipped when formatting the UTF8 sequence. We include
  // \xc2\xa1 "¡" in the sequence to verify that we do still emit
  // valid UTF8 characters.
  const char* kInvalidUtf8 = "hello\xc2 \xc2\xa1\xc3 hello";
  std::stringstream output;
  JsonFormatter formatter(&output, NULL);
  Argument url(Argument::URL, kInvalidUtf8);
  formatter.AddChild(not_localized("$1"), url);
  formatter.Done();
  std::string result = output.str();
  EXPECT_EQ("[\n{\"format\":[{\"type\":\"url\",\"value\":"
            "\"hello ¡ hello\""
            "}]}]\n",
            result);
}

}  // namespace
