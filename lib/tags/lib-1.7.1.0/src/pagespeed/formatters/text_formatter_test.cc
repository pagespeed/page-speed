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

#include "pagespeed/formatters/text_formatter.h"
#include "testing/gtest/include/gtest/gtest.h"

using pagespeed::Argument;
using pagespeed::Formatter;
using pagespeed::formatters::TextFormatter;

namespace {

class DummyTestRule : public pagespeed::Rule {
 public:
  explicit DummyTestRule(const char* header) : header_(header) {}

  virtual const char* name() const { return "Dummy Test Rule"; }
  virtual const char* header() const { return header_; }
  virtual const char* documentation_url() const { return ""; }
  virtual bool AppendResults(const pagespeed::PagespeedInput& input,
                             pagespeed::ResultProvider* provider) {
    return true;
  }
  virtual void FormatResults(const pagespeed::ResultVector& results,
                             Formatter* formatter) {}
 private:
  const char* header_;
};

TEST(TextFormatterTest, BasicTest) {
  std::stringstream output;
  TextFormatter formatter(&output);
  formatter.AddChild("foo");
  formatter.AddChild("bar");
  formatter.Done();
  std::string result = output.str();
  EXPECT_EQ("foo\nbar\n", result);
}

TEST(TextFormatterTest, BasicHeaderTest) {
  std::stringstream output;
  DummyTestRule rule1("foo");
  DummyTestRule rule2("bar");
  TextFormatter formatter(&output);
  formatter.AddHeader(rule1, 0);
  formatter.AddHeader(rule2, 1);
  formatter.Done();
  std::string result = output.str();
  EXPECT_EQ("_foo_ (score=0)\n_bar_ (score=1)\n", result);
}

TEST(TextFormatterTest, TreeTest) {
  std::stringstream output;
  TextFormatter formatter(&output);
  DummyTestRule rule("l1-1");
  Formatter* level1 = formatter.AddHeader(rule, -1);
  Formatter* level2 = level1->AddChild("l2-1");
  Formatter* level3 = level2->AddChild("l3-1");
  level3->AddChild("l4-1");
  level3->AddChild("l4-2");
  level3 = level2->AddChild("l3-2");
  level3->AddChild("l4-3");
  level3->AddChild("l4-4");
  formatter.Done();
  std::string result = output.str();
  EXPECT_EQ("_l1-1_ (score=n/a)\n"
            "  l2-1\n"
            "    * l3-1\n"
            "      * l4-1\n"
            "      * l4-2\n"
            "    * l3-2\n"
            "      * l4-3\n"
            "      * l4-4\n", result);
}

TEST(TextFormatterTest, ArgumentTypesTest) {
  std::stringstream output;
  TextFormatter formatter(&output);
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
  EXPECT_EQ("1.5KiB\n"
            "42\n"
            "test\n"
            "http://test.com/\n",
            result);
}

TEST(TextFormatterTest, ArgumentListTest) {
  std::stringstream output;
  TextFormatter formatter(&output);
  Argument bytes_arg(Argument::BYTES, 1536);
  Argument int_arg(Argument::INTEGER, 42);
  Argument string_arg(Argument::STRING, "test");
  Argument url_arg(Argument::URL, "http://test.com/");
  formatter.AddChild("");
  formatter.AddChild("$1", bytes_arg);
  formatter.AddChild("$1 $2", bytes_arg, int_arg);
  formatter.AddChild("$1 $2 $3", bytes_arg, int_arg, string_arg);
  formatter.AddChild("$1 $2 $3 $4", bytes_arg, int_arg, string_arg, url_arg);
  std::string result = output.str();
  EXPECT_EQ("\n"
            "1.5KiB\n"
            "1.5KiB 42\n"
            "1.5KiB 42 test\n"
            "1.5KiB 42 test http://test.com/\n",
            result);
}

}  // namespace
