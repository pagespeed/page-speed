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

#include <string>

#include "base/string_piece.h"
#include "pagespeed/jsminify/js_minify.h"
#include "testing/gtest/include/gtest/gtest.h"

using namespace pagespeed::jsminify;

namespace {

// This sample code comes from Douglas Crockford's jsmin example.
const char* kBeforeCompilation =
    "// is.js\n"
    "\n"
    "// (c) 2001 Douglas Crockford\n"
    "// 2001 June 3\n"
    "\n"
    "\n"
    "// is\n"
    "\n"
    "// The -is- object is used to identify the browser.  "
    "Every browser edition\n"
    "// identifies itself, but there is no standard way of doing it, "
    "and some of\n"
    "// the identification is deceptive. This is because the authors of web\n"
    "// browsers are liars. For example, Microsoft's IE browsers claim to be\n"
    "// Mozilla 4. Netscape 6 claims to be version 5.\n"
    "\n"
    "var is = {\n"
    "    ie:      navigator.appName == 'Microsoft Internet Explorer',\n"
    "    java:    navigator.javaEnabled(),\n"
    "    ns:      navigator.appName == 'Netscape',\n"
    "    ua:      navigator.userAgent.toLowerCase(),\n"
    "    version: parseFloat(navigator.appVersion.substr(21)) ||\n"
    "             parseFloat(navigator.appVersion),\n"
    "    win:     navigator.platform == 'Win32'\n"
    "}\n"
    "is.mac = is.ua.indexOf('mac') >= 0;\n"
    "if (is.ua.indexOf('opera') >= 0) {\n"
    "    is.ie = is.ns = false;\n"
    "    is.opera = true;\n"
    "}\n"
    "if (is.ua.indexOf('gecko') >= 0) {\n"
    "    is.ie = is.ns = false;\n"
    "    is.gecko = true;\n"
    "}\n";

const char* kAfterCompilation =
    "var is={ie:navigator.appName=='Microsoft Internet Explorer',"
    "java:navigator.javaEnabled(),ns:navigator.appName=='Netscape',"
    "ua:navigator.userAgent.toLowerCase(),version:parseFloat("
    "navigator.appVersion.substr(21))||parseFloat(navigator.appVersion)"
    ",win:navigator.platform=='Win32'}\n"
    "is.mac=is.ua.indexOf('mac')>=0;if(is.ua.indexOf('opera')>=0){"
    "is.ie=is.ns=false;is.opera=true;}\n"
    "if(is.ua.indexOf('gecko')>=0){is.ie=is.ns=false;is.gecko=true;}";

TEST(JsMinifyTest, Basic) {
  std::string output;
  ASSERT_TRUE(MinifyJs(kBeforeCompilation, &output));
  ASSERT_EQ(kAfterCompilation, output);

  int minimized_size = -1;
  ASSERT_TRUE(GetMinifiedJsSize(kBeforeCompilation, &minimized_size));
  ASSERT_EQ(static_cast<int>(strlen(kAfterCompilation)), minimized_size);
}

TEST(JsMinifyTest, AlreadyMinified) {
  std::string output;
  ASSERT_TRUE(MinifyJs(kAfterCompilation, &output));
  ASSERT_EQ(kAfterCompilation, output);

  int minimized_size = -1;
  ASSERT_TRUE(GetMinifiedJsSize(kAfterCompilation, &minimized_size));
  ASSERT_EQ(static_cast<int>(strlen(kAfterCompilation)), minimized_size);
}

TEST(JsMinifyTest, ErrorUnclosedComment) {
  std::string input = "/* not valid javascript";
  std::string output;
  ASSERT_FALSE(MinifyJs(input, &output));
  ASSERT_TRUE(output.empty());

  int minimized_size = -1;
  ASSERT_FALSE(GetMinifiedJsSize(input, &minimized_size));
  ASSERT_EQ(-1, minimized_size);
}

TEST(JsMinifyTest, ErrorUnclosedString) {
  std::string input = "\"not valid javascript";
  std::string output;
  ASSERT_FALSE(MinifyJs(input, &output));
  ASSERT_TRUE(output.empty());

  int minimized_size = -1;
  ASSERT_FALSE(GetMinifiedJsSize(input, &minimized_size));
  ASSERT_EQ(-1, minimized_size);
}

TEST(JsMinifyTest, ErrorUnclosedRegex) {
  std::string input = "/not_valid_javascript";
  std::string output;
  ASSERT_FALSE(MinifyJs(input, &output));
  ASSERT_TRUE(output.empty());

  int minimized_size = -1;
  ASSERT_FALSE(GetMinifiedJsSize(input, &minimized_size));
  ASSERT_EQ(-1, minimized_size);
}

TEST(JsMinifyTest, SignedCharDoesntSignExtend) {
  const unsigned char input[] = { 0xff, 0x00 };
  const char* input_nosign = reinterpret_cast<const char*>(input);
  std::string output;
  ASSERT_TRUE(MinifyJs(input_nosign, &output));
  ASSERT_EQ(input_nosign, output);

  int minimized_size = -1;
  ASSERT_TRUE(GetMinifiedJsSize(input_nosign, &minimized_size));
  ASSERT_EQ(1, minimized_size);
}

TEST(JsMinifyTest, DealWithCrlf) {
  std::string input = "var x = 1;\r\nvar y = 2;";
  std::string expected = "var x=1;var y=2;";
  std::string output;
  ASSERT_TRUE(MinifyJs(input, &output));
  ASSERT_EQ(expected, output);
}

TEST(JsMinifyTest, DealWithTabs) {
  std::string input = "var x = 1;\n\tvar y = 2;";
  std::string expected = "var x=1;var y=2;";
  std::string output;
  ASSERT_TRUE(MinifyJs(input, &output));
  ASSERT_EQ(expected, output);
}

TEST(JsMinifyTest, EscapedCrlfInStringLiteral) {
  std::string input = "var x = 'foo\\\r\nbar';";
  std::string expected = "var x='foo\\\r\nbar';";
  std::string output;
  ASSERT_TRUE(MinifyJs(input, &output));
  ASSERT_EQ(expected, output);
}

TEST(JsMinifyTest, EmptyInput) {
  std::string output;
  ASSERT_TRUE(MinifyJs("", &output));
  ASSERT_EQ("", output);
}

// See http://code.google.com/p/page-speed/issues/detail?id=198
TEST(JsMinifyTest, LeaveIEConditionalCompilationComments) {
  const std::string input =
      "/*@cc_on\n"
      "  /*@if (@_win32)\n"
      "    document.write('IE');\n"
      "  @else @*/\n"
      "    document.write('other');\n"
      "  /*@end\n"
      "@*/";
  const std::string expected =
      "/*@cc_on\n"
      "  /*@if (@_win32)\n"
      "    document.write('IE');\n"
      "  @else @*/\n"
      "document.write('other');/*@end\n"
      "@*/";
  std::string output;
  ASSERT_TRUE(MinifyJs(input, &output));
  ASSERT_EQ(expected, output);
}

TEST(JsMinifyTest, DoNotJoinPlusses) {
  std::string input = "var x = 'date=' + +new Date();";
  std::string expected = "var x='date='+ +new Date();";
  std::string output;
  ASSERT_TRUE(MinifyJs(input, &output));
  ASSERT_EQ(expected, output);
}

TEST(JsMinifyTest, DoJoinBangs) {
  std::string input = "var x = ! ! y;";
  std::string expected = "var x=!!y;";
  std::string output;
  ASSERT_TRUE(MinifyJs(input, &output));
  ASSERT_EQ(expected, output);
}

// See http://code.google.com/p/page-speed/issues/detail?id=242
TEST(JsMinifyTest, RemoveSurroundingSgmlComment) {
  std::string input = "<!--\nvar x = 42;\n//-->";
  std::string expected = "var x=42;";
  std::string output;
  ASSERT_TRUE(MinifyJs(input, &output));
  ASSERT_EQ(expected, output);
}

TEST(JsMinifyTest, RemoveSurroundingSgmlCommentWithoutSlashSlash) {
  std::string input = "<!--\nvar x = 42;\n-->\n";
  std::string expected = "var x=42;";
  std::string output;
  ASSERT_TRUE(MinifyJs(input, &output));
  ASSERT_EQ(expected, output);
}

// See http://code.google.com/p/page-speed/issues/detail?id=242
TEST(JsMinifyTest, SgmlLineComment) {
  std::string input = "var x = 42; <!-- comment\nvar y = 17;";
  std::string expected = "var x=42;var y=17;";
  std::string output;
  ASSERT_TRUE(MinifyJs(input, &output));
  ASSERT_EQ(expected, output);
}

TEST(JsMinifyTest, RemoveSgmlCommentCloseOnOwnLine1) {
  std::string input = "var x = 42;\n    --> \n";
  std::string expected = "var x=42;";
  std::string output;
  ASSERT_TRUE(MinifyJs(input, &output));
  ASSERT_EQ(expected, output);
}

TEST(JsMinifyTest, RemoveSgmlCommentCloseOnOwnLine2) {
  std::string input = "-->\nvar x = 42;\n";
  std::string expected = "var x=42;";
  std::string output;
  ASSERT_TRUE(MinifyJs(input, &output));
  ASSERT_EQ(expected, output);
}

TEST(JsMinifyTest, DoNotRemoveSgmlCommentCloseInMidLine) {
  std::string input = "var x = 42; --> \n";
  std::string expected = "var x=42;-->";
  std::string output;
  ASSERT_TRUE(MinifyJs(input, &output));
  ASSERT_EQ(expected, output);
}

TEST(JsMinifyTest, DoNotCreateLineComment) {
  // Yes, this is legal code.  It sets x to NaN.
  std::string input = "var x = 42 / /foo/;\n";
  std::string expected = "var x=42/ /foo/;";
  std::string output;
  ASSERT_TRUE(MinifyJs(input, &output));
  ASSERT_EQ(expected, output);
}

TEST(JsMinifyTest, DoNotCreateSgmlLineComment) {
  // Yes, this is legal code.  It tests if x is less than not(decrement y).
  std::string input = "if (x < ! --y) { x = 0; }\n";
  std::string expected = "if(x< ! --y){x=0;}";
  std::string output;
  ASSERT_TRUE(MinifyJs(input, &output));
  ASSERT_EQ(expected, output);
}

TEST(JsMinifyTest, TrickyRegexLiteral) {
  // The first assignment is two divisions; the second assignment is a regex
  // literal.  JSMin gets this wrong (it removes whitespace from the regex).
  std::string input = "var x = a[0] / b /i;\n var y = a[0] + / b /i;";
  std::string expected = "var x=a[0]/b/i;var y=a[0]+/ b /i;";
  std::string output;
  ASSERT_TRUE(MinifyJs(input, &output));
  ASSERT_EQ(expected, output);
}

const char kCrashTestString[] =
    "var x = 'asd \\' lse'\n"
    "var y /*comment*/ = /regex/\n"
    "var z = \"x =\" + x\n";

TEST(JsMinifyTest, DoNotCrash) {
  // Run on all possible prefixes of kCrashTestString.  We don't care about the
  // result; we just want to make sure it doesn't crash.
  for (int i = 0, size = sizeof(kCrashTestString); i <= size; ++i) {
    std::string input(kCrashTestString, i);
    std::string output;
    MinifyJs(input, &output);
  }
}

}  // namespace
