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

#include <vector>

#include "pagespeed/core/string_util.h"
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

  for (size_t i = 0; i < ARRAYSIZE_UNSAFE(cases); ++i) {
    int output = 0;
    EXPECT_EQ(cases[i].success, StringToInt(cases[i].input, &output));
    EXPECT_EQ(cases[i].output, output);
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

TEST(StringUtilTest, GetStringFWithOffsets) {
  std::vector<std::string> subst;
  subst.push_back("1");
  subst.push_back("2");
  std::vector<size_t> offsets;

  ReplaceStringPlaceholders("Hello, $1. Your number is $2.",
                            subst,
                            &offsets);
  EXPECT_EQ(2U, offsets.size());
  EXPECT_EQ(7U, offsets[0]);
  EXPECT_EQ(25U, offsets[1]);
  offsets.clear();

  ReplaceStringPlaceholders("Hello, $2. Your number is $1.",
                            subst,
                            &offsets);
  EXPECT_EQ(2U, offsets.size());
  EXPECT_EQ(25U, offsets[0]);
  EXPECT_EQ(7U, offsets[1]);
  offsets.clear();
}

TEST(StringUtilTest, ReplaceStringPlaceholdersTooFew) {
  // Test whether replacestringplaceholders works as expected when there
  // are fewer inputs than outputs.

  std::vector<std::string> subst;
  subst.push_back("9a");
  subst.push_back("8b");
  subst.push_back("7c");

  std::string formatted =
      ReplaceStringPlaceholders(
         "$1a,$2b,$3c,$4d,$5e,$6f,$1g,$2h,$3i", subst, NULL);

  EXPECT_EQ(formatted, "9aa,8bb,7cc,d,e,f,9ag,8bh,7ci");
}

TEST(StringUtilTest, ReplaceStringPlaceholders) {
  std::vector<std::string> subst;
  subst.push_back("9a");
  subst.push_back("8b");
  subst.push_back("7c");
  subst.push_back("6d");
  subst.push_back("5e");
  subst.push_back("4f");
  subst.push_back("3g");
  subst.push_back("2h");
  subst.push_back("1i");

  std::string formatted =
      ReplaceStringPlaceholders(
          "$1a,$2b,$3c,$4d,$5e,$6f,$7g,$8h,$9i", subst, NULL);

  EXPECT_EQ(formatted, "9aa,8bb,7cc,6dd,5ee,4ff,3gg,2hh,1ii");
}

TEST(StringUtilTest, ReplaceStringPlaceholdersMoreThan9Replacements) {
  std::vector<std::string> subst;
  subst.push_back("9a");
  subst.push_back("8b");
  subst.push_back("7c");
  subst.push_back("6d");
  subst.push_back("5e");
  subst.push_back("4f");
  subst.push_back("3g");
  subst.push_back("2h");
  subst.push_back("1i");
  subst.push_back("0j");
  subst.push_back("-1k");
  subst.push_back("-2l");
  subst.push_back("-3m");
  subst.push_back("-4n");

  std::string formatted =
      ReplaceStringPlaceholders(
          "$1a,$2b,$3c,$4d,$5e,$6f,$7g,$8h,$9i,"
          "$10j,$11k,$12l,$13m,$14n,$1", subst, NULL);

  EXPECT_EQ(formatted, "9aa,8bb,7cc,6dd,5ee,4ff,3gg,2hh,"
                                    "1ii,0jj,-1kk,-2ll,-3mm,-4nn,9a");

}

TEST(StringUtilTest, StdStringReplaceStringPlaceholders) {
  std::vector<std::string> subst;
  subst.push_back("9a");
  subst.push_back("8b");
  subst.push_back("7c");
  subst.push_back("6d");
  subst.push_back("5e");
  subst.push_back("4f");
  subst.push_back("3g");
  subst.push_back("2h");
  subst.push_back("1i");

  std::string formatted =
      ReplaceStringPlaceholders(
          "$1a,$2b,$3c,$4d,$5e,$6f,$7g,$8h,$9i", subst, NULL);

  EXPECT_EQ(formatted, "9aa,8bb,7cc,6dd,5ee,4ff,3gg,2hh,1ii");
}

TEST(StringUtilTest, ReplaceStringPlaceholdersConsecutiveDollarSigns) {
  std::vector<std::string> subst;
  subst.push_back("a");
  subst.push_back("b");
  subst.push_back("c");
  EXPECT_EQ(ReplaceStringPlaceholders("$$1 $$$2 $$$$3", subst, NULL),
            "$1 $$2 $$$3");
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
#if defined(OS_WIN)
  sprintf_s(ref, kRefSize, fmt, src, src, src, src, src, src, src);
#elif defined(OS_POSIX)
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

// TODO(evanm): what's the proper cross-platform test here?
#if defined(OS_WIN)
// sprintf in Visual Studio fails when given U+FFFF. This tests that the
// failure case is gracefuly handled.
TEST(StringUtilTest, Invalid) {
  wchar_t invalid[2];
  invalid[0] = 0xffff;
  invalid[1] = 0;

  std::wstring out;
  SStringPrintf(&out, L"%ls", invalid);
  EXPECT_STREQ(L"", out.c_str());
}
#endif

}  // namespace
