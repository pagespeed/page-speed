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

#include "pagespeed/core/string_util.h"

#include <map>
#include <string>
#include <vector>

#include "testing/gtest/include/gtest/gtest.h"

namespace {

using namespace pagespeed::string_util;

TEST(StringUtilTest, CaseInsensitiveStringComparator) {
  EXPECT_TRUE(CaseInsensitiveStringComparator()("bar", "foo"));
  EXPECT_FALSE(CaseInsensitiveStringComparator()("foo", "bar"));
  EXPECT_TRUE(CaseInsensitiveStringComparator()("BAR", "FOO"));
  EXPECT_FALSE(CaseInsensitiveStringComparator()("FOO", "BAR"));
  EXPECT_TRUE(CaseInsensitiveStringComparator()("bar", "FOO"));
  EXPECT_FALSE(CaseInsensitiveStringComparator()("FOO", "bar"));
  EXPECT_TRUE(CaseInsensitiveStringComparator()("BAR", "foo"));
  EXPECT_FALSE(CaseInsensitiveStringComparator()("foo", "BAR"));

  EXPECT_FALSE(CaseInsensitiveStringComparator()("bar", "BAR"));
  EXPECT_FALSE(CaseInsensitiveStringComparator()("BAR", "bar"));
  EXPECT_FALSE(CaseInsensitiveStringComparator()("BaR", "bAr"));
  EXPECT_FALSE(CaseInsensitiveStringComparator()("bAr", "BaR"));
}

TEST(StringUtilTest, StringCaseEqual) {
  EXPECT_TRUE(StringCaseEqual("", ""));
  EXPECT_FALSE(StringCaseEqual("", "foo"));
  EXPECT_FALSE(StringCaseEqual("foo", ""));
  EXPECT_FALSE(StringCaseEqual("foobar", "fobar"));
  EXPECT_TRUE(StringCaseEqual("foobar", "foobar"));
  EXPECT_TRUE(StringCaseEqual("foobar", "FOOBAR"));
  EXPECT_TRUE(StringCaseEqual("FOOBAR", "foobar"));
  EXPECT_TRUE(StringCaseEqual("fOoBaR", "FoObAr"));
}

TEST(StringUtilTest, StringCaseStartsWith) {
  EXPECT_FALSE(StringCaseStartsWith("foobar", "fob"));
  EXPECT_TRUE(StringCaseStartsWith("foobar", "foobar"));
  EXPECT_TRUE(StringCaseStartsWith("foobar", "foo"));
  EXPECT_TRUE(StringCaseStartsWith("foobar", "FOO"));
  EXPECT_TRUE(StringCaseStartsWith("FOOBAR", "foo"));
  EXPECT_TRUE(StringCaseStartsWith("fOoBaR", "FoO"));
  EXPECT_FALSE(StringCaseStartsWith("zzz", "zzzz"));
}

TEST(StringUtilTest, StringCaseEndsWith) {
  EXPECT_FALSE(StringCaseEndsWith("foobar", "baar"));
  EXPECT_TRUE(StringCaseEndsWith("foobar", "foobar"));
  EXPECT_TRUE(StringCaseEndsWith("foobar", "bar"));
  EXPECT_TRUE(StringCaseEndsWith("foobar", "BAR"));
  EXPECT_TRUE(StringCaseEndsWith("FOOBAR", "bar"));
  EXPECT_TRUE(StringCaseEndsWith("fOoBaR", "bAr"));
  EXPECT_FALSE(StringCaseEndsWith("zzz", "zzzz"));
  EXPECT_TRUE(StringCaseEndsWith("foobar", ""));
  EXPECT_FALSE(StringCaseEndsWith("", "foo"));
}

TEST(StringUtilTest, IntToString) {
  EXPECT_STREQ("0", IntToString(0).c_str());
  EXPECT_STREQ("1", IntToString(1).c_str());
  EXPECT_STREQ("11", IntToString(11).c_str());
  // Check octal values ie. 0xyz
  // Good to note this test case even though
  // number conversion is done at compile-time.
  EXPECT_STREQ("9", IntToString(011).c_str());
  EXPECT_STREQ("17", IntToString(0x11).c_str());
  // Check negative numbers
  EXPECT_STREQ("-123", IntToString(-123).c_str());
  EXPECT_STREQ("-99999", IntToString(-99999).c_str());
}

TEST(StringUtilTest, StringToInt) {
  static const struct {
    std::string input;
    int output;
    bool success;
  } cases[] = {
    {"0", 0, true},
    {"42", 42, true},
    {"42\x99", 42, false},
    {"\x99" "42\x99", 0, false},
    {"-2147483648", INT_MIN, true},
    {"2147483647", INT_MAX, true},
    {"", 0, false},
    {"  ", 0, false},
    {" 42", 42, true},
    {"42 ", 42, true},
    {"\t\n\v\f\r 42", 42, true},
    {"blah42", 0, false},
    {"42blah", 42, false},
    {"blah42blah", 0, false},
    {"-273.15", -273, false},
    {"+98.6", 98, false},
    {"--123", 0, false},
    {"++123", 0, false},
    {"-+123", 0, false},
    {"+-123", 0, false},
    {"-", 0, false},
    {"-2147483649", INT_MIN, false},
    {"-99999999999", INT_MIN, false},
    {"2147483648", INT_MAX, false},
    {"99999999999", INT_MAX, false},
    {" 123 ", 123, true},
    {" -123 ", -123, true},
  };

  static const size_t kCasesSize = sizeof(cases) / sizeof(cases[0]);

  for (size_t i = 0; i < kCasesSize; ++i) {
    int output = 0;
    EXPECT_EQ(cases[i].success, StringToInt(cases[i].input, &output))
        << "Input: '" << cases[i].input << "'";
    EXPECT_EQ(cases[i].output, output)
        << "Input: '" << cases[i].input << "'";
  }

}

// Test for JoinString
TEST(StringUtilTest, JoinString) {
  std::vector<std::string> in;
  EXPECT_EQ("", JoinString(in, ','));

  in.push_back("a");
  EXPECT_EQ("a", JoinString(in, ','));

  in.push_back("b");
  in.push_back("c");
  EXPECT_EQ("a,b,c", JoinString(in, ','));

  in.push_back("");
  EXPECT_EQ("a,b,c,", JoinString(in, ','));
  in.push_back(" ");
  EXPECT_EQ("a|b|c|| ", JoinString(in, '|'));
}

TEST(StringUtilTest, ReplaceStringPlaceholdersSimple) {
  std::map<std::string, std::string> subst;
  subst["FOO_BAR"] = "Hello";
  subst["BAZ_1"] = "world";
  EXPECT_EQ("Hello, world!",
            ReplaceStringPlaceholders("%(FOO_BAR)s, %(BAZ_1)s!", subst));
}

TEST(StringUtilTest, ReplaceStringPlaceholdersMany) {
  std::map<std::string, std::string> subst;
  subst["A"] = "a";
  subst["B"] = "b";
  subst["C"] = "c";
  subst["D"] = "d";
  subst["E"] = "e";
  subst["F"] = "f";
  subst["G"] = "g";
  subst["H"] = "h";
  subst["I"] = "i";
  subst["J"] = "j";
  subst["K"] = "k";
  subst["L"] = "l";
  subst["M"] = "m";
  EXPECT_EQ("mlkjihgfedcba", ReplaceStringPlaceholders(
      "%(M)s%(L)s%(K)s%(J)s%(I)s%(H)s%(G)s%(F)s%(E)s%(D)s%(C)s%(B)s%(A)s",
      subst));
}

TEST(StringUtilTest, ReplaceStringPlaceholdersEscapedPercents) {
  std::map<std::string, std::string> subst;
  subst["INT"] = "99";
  EXPECT_EQ("This is 99% awesome.",
            ReplaceStringPlaceholders("This is %(INT)s%% awesome.", subst));
  EXPECT_EQ("This is only 98% (less than 99%) awesome.",
            ReplaceStringPlaceholders(
                "This is only 98%% (less than %(INT)s%%) awesome.", subst));
}

// If a placeholder isn't closed properly (for example, if we forgot the 's'
// after the close paren), that's an error.
TEST(StringUtilTest, ReplaceStringPlaceholdersUnclosedPlaceholder) {
  std::map<std::string, std::string> subst;
  subst["INT"] = "5";
  EXPECT_DEBUG_DEATH(
      ReplaceStringPlaceholders("There are %(INT results.", subst),
      "Unclosed format placeholder");
  EXPECT_DEBUG_DEATH(
      ReplaceStringPlaceholders("This are %(INT) results.", subst),
      "Unclosed format placeholder");
}

// If the format string contains a placeholder that isn't in the map, that's an
// error.
TEST(StringUtilTest, ReplaceStringPlaceholdersPlaceholderNotInMap) {
  std::map<std::string, std::string> subst;
  subst["FOO"] = "5";
  EXPECT_DEBUG_DEATH(
      ReplaceStringPlaceholders("Hello %(FOO)s %(BAR)s.", subst),
      "No such placeholder key: BAR");
}

// A percent sign in the format string must be followed by either an open paren
// or another percent sign.  Anything else is an error.
TEST(StringUtilTest, ReplaceStringPlaceholdersInvalidEscape) {
  std::map<std::string, std::string> subst;
  subst["BAR"] = "42";
  EXPECT_DEBUG_DEATH(
      ReplaceStringPlaceholders("Foo %(BAR)s %t baz.", subst),
      "Invalid format escape: %t");
}

TEST(StringUtilTest, StringPrintfEmpty) {
  EXPECT_EQ("", StringPrintf("%s", ""));
}

TEST(StringUtilTest, StringPrintfMisc) {
  EXPECT_EQ("123hello w", StringPrintf("%3d%2s %1c", 123, "hello", 'w'));
}

// Make sure that lengths exactly around the initial buffer size are handled
// correctly.
TEST(StringUtilTest, StringPrintfBounds) {
  const int kSrcLen = 1026;
  char src[kSrcLen];
  for (size_t i = 0; i < arraysize(src); i++)
    src[i] = 'A';

  for (int i = 1; i < 3; i++) {
    src[kSrcLen - i] = 0;
    std::string out;
    out = StringPrintf("%s", src);
    EXPECT_STREQ(src, out.c_str());
  }
}

// Test very large sprintfs that will cause the buffer to grow.
TEST(StringUtilTest, Grow) {
  char src[1026];
  for (size_t i = 0; i < arraysize(src); i++)
    src[i] = 'A';
  src[1025] = 0;

  const char* fmt = "%sB%sB%sB%sB%sB%sB%s";

  std::string out;
  out = StringPrintf(fmt, src, src, src, src, src, src, src);

  const int kRefSize = 320000;
  char* ref = new char[kRefSize];
#if defined(_WIN32)
  sprintf_s(ref, kRefSize, fmt, src, src, src, src, src, src, src);
#else
  snprintf(ref, kRefSize, fmt, src, src, src, src, src, src, src);
#endif

  EXPECT_STREQ(ref, out.c_str());
  delete[] ref;
}

// Test the boundary condition for the size of the string_util's
// internal buffer.
TEST(StringUtilTest, GrowBoundary) {
  const int string_util_buf_len = 1024;
  // Our buffer should be one larger than the size of StringAppendVT's stack
  // buffer.
  const int buf_len = string_util_buf_len + 1;
  char src[buf_len + 1];  // Need extra one for NULL-terminator.
  for (int i = 0; i < buf_len; ++i)
    src[i] = 'a';
  src[buf_len] = 0;

  std::string out;
  out = StringPrintf("%s", src);

  EXPECT_STREQ(src, out.c_str());
}

static const struct {
  const char*    src;
  const char*    dst;
} lowercase_cases[] = {
  {"FoO", "foo"},
  {"foo", "foo"},
  {"FOO", "foo"},
};

static const size_t kLowercaseCasesSize =
    sizeof(lowercase_cases) / sizeof(lowercase_cases[0]);

TEST(StringUtilTest, LowerCaseEqualsASCII) {
  for (size_t i = 0; i < kLowercaseCasesSize; ++i) {
    EXPECT_TRUE(LowerCaseEqualsASCII(lowercase_cases[i].src,
                                     lowercase_cases[i].dst));
  }
}

TEST(StringUtilTest, ContainsOnlyWhitespaceASCII) {
  EXPECT_TRUE(ContainsOnlyWhitespaceASCII(""));
  EXPECT_TRUE(ContainsOnlyWhitespaceASCII(" "));
  EXPECT_TRUE(ContainsOnlyWhitespaceASCII("\t"));
  EXPECT_TRUE(ContainsOnlyWhitespaceASCII("\t \r \n  "));
  EXPECT_FALSE(ContainsOnlyWhitespaceASCII("a"));
  EXPECT_FALSE(ContainsOnlyWhitespaceASCII("\thello\r \n  "));
}

}  // namespace
