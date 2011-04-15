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
#include "testing/gtest/include/gtest/gtest.h"

namespace {

using namespace pagespeed::string_util;

TEST(CaseInsensitiveStringComparatorTest, Compare) {
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

TEST(StringCaseTest, TestStringCaseEqual) {
  EXPECT_FALSE(StringCaseEqual("foobar", "fobar"));
  EXPECT_TRUE(StringCaseEqual("foobar", "foobar"));
  EXPECT_TRUE(StringCaseEqual("foobar", "FOOBAR"));
  EXPECT_TRUE(StringCaseEqual("FOOBAR", "foobar"));
  EXPECT_TRUE(StringCaseEqual("fOoBaR", "FoObAr"));
}

TEST(StringCaseTest, TestStringCaseStartsWith) {
  EXPECT_FALSE(StringCaseStartsWith("foobar", "fob"));
  EXPECT_TRUE(StringCaseStartsWith("foobar", "foobar"));
  EXPECT_TRUE(StringCaseStartsWith("foobar", "foo"));
  EXPECT_TRUE(StringCaseStartsWith("foobar", "FOO"));
  EXPECT_TRUE(StringCaseStartsWith("FOOBAR", "foo"));
  EXPECT_TRUE(StringCaseStartsWith("fOoBaR", "FoO"));
  EXPECT_FALSE(StringCaseStartsWith("zzz", "zzzz"));
}

TEST(StringCaseTest, TestStringCaseEndsWith) {
  EXPECT_FALSE(StringCaseEndsWith("foobar", "baar"));
  EXPECT_TRUE(StringCaseEndsWith("foobar", "foobar"));
  EXPECT_TRUE(StringCaseEndsWith("foobar", "bar"));
  EXPECT_TRUE(StringCaseEndsWith("foobar", "BAR"));
  EXPECT_TRUE(StringCaseEndsWith("FOOBAR", "bar"));
  EXPECT_TRUE(StringCaseEndsWith("fOoBaR", "bAr"));
  EXPECT_FALSE(StringCaseEndsWith("zzz", "zzzz"));
}

}  // namespace
