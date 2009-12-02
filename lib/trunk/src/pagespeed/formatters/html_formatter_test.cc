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

#include "pagespeed/formatters/html_formatter.h"
#include "testing/gtest/include/gtest/gtest.h"

using pagespeed::Argument;
using pagespeed::Formatter;
using pagespeed::formatters::HtmlFormatter;

namespace {

class DummyTestRule : public pagespeed::Rule {
 public:
  explicit DummyTestRule(const char* header) : header_(header) {}

  virtual const char* name() const { return "Dummy Test Rule"; }
  virtual const char* header() const { return header_; }
  virtual bool AppendResults(const pagespeed::PagespeedInput& input,
                             pagespeed::Results* results) { return true; }
  virtual void FormatResults(const pagespeed::ResultVector& results,
                             Formatter* formatter) {}
 private:
  const char* header_;
};

TEST(HtmlFormatterTest, BasicTest) {
  std::stringstream output;
  HtmlFormatter formatter(&output);
  formatter.AddChild("foo");
  formatter.AddChild("bar");
  formatter.Done();
  std::string result = output.str();
  EXPECT_EQ("<h1>foo</h1>\n<h1>bar</h1>\n", result);
}

TEST(HtmlFormatterTest, BasicHeaderTest) {
  std::stringstream output;
  HtmlFormatter formatter(&output);
  DummyTestRule rule1("head");
  DummyTestRule rule2("head2");
  Formatter* child_formatter = formatter.AddHeader(rule1, 42);
  child_formatter->AddChild("foo");
  child_formatter->AddChild("bar");
  formatter.AddHeader(rule2, 23);
  formatter.Done();
  std::string result = output.str();
  EXPECT_EQ("<h1>42 head</h1>\n"
            " <h2>foo</h2>\n"
            " <h2>bar</h2>\n"
            "<h1>23 head2</h1>\n", result);
}

TEST(HtmlFormatterTest, TreeTest) {
  std::stringstream output;
  HtmlFormatter formatter(&output);
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
  EXPECT_EQ("<h1>l1-1</h1>\n"
            " <h2>l2-1</h2>\n"
            " <ul>\n"
            "  <li>l3-1</li>\n"
            "  <ul>\n"
            "   <li>l4-1</li>\n"
            "   <li>l4-2</li>\n"
            "  </ul>\n"
            "  <li>l3-2</li>\n"
            "  <ul>\n"
            "   <li>l4-3</li>\n"
            "   <li>l4-4</li>\n"
            "  </ul>\n"
            " </ul>\n", result);
}

TEST(HtmlFormatterTest, ArgumentTypesTest) {
  std::stringstream output;
  HtmlFormatter formatter(&output);
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
  EXPECT_EQ("<h1>1.5KiB</h1>\n"
            "<h1>42</h1>\n"
            "<h1>test</h1>\n"
            "<h1><a href=\"http://test.com/\">http://test.com/</a></h1>\n",
            result);
}

TEST(HtmlFormatterTest, ArgumentListTest) {
  std::stringstream output;
  HtmlFormatter formatter(&output);
  Argument bytes_arg(Argument::BYTES, 1536);
  Argument int_arg(Argument::INTEGER, 42);
  Argument string_arg(Argument::STRING, "test");
  Argument url_arg(Argument::URL, "http://test.com/");
  formatter.AddChild("");
  formatter.AddChild("$1", bytes_arg);
  formatter.AddChild("$1 $2", bytes_arg, int_arg);
  formatter.AddChild("$1 $2 $3", bytes_arg, int_arg, string_arg);
  formatter.AddChild("$1 $2 $3 $4", bytes_arg, int_arg, string_arg, url_arg);
  formatter.Done();
  std::string result = output.str();
  EXPECT_EQ("<h1></h1>\n"
            "<h1>1.5KiB</h1>\n"
            "<h1>1.5KiB 42</h1>\n"
            "<h1>1.5KiB 42 test</h1>\n"
            "<h1>1.5KiB 42 test"
            " <a href=\"http://test.com/\">http://test.com/</a></h1>\n",
            result);
}

}  // namespace
