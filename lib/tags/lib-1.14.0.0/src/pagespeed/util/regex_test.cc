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

#include "pagespeed/util/regex.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

const char *kRegex1 = "a+b+cd+e";
const char *kRegex2 = "(a+b+cd+e|f+g+hi+j)";
const char *kRegexInvalid = "?";

TEST(RegexTest, Basic) {
  pagespeed::RE re;
  ASSERT_FALSE(re.is_valid());
  ASSERT_TRUE(re.Init(kRegex1));
  ASSERT_TRUE(re.is_valid());

  ASSERT_TRUE(re.PartialMatch("abcde"));
  ASSERT_TRUE(re.PartialMatch("padding abcde padding"));
  ASSERT_TRUE(re.PartialMatch("aaabbbcdde"));

  ASSERT_FALSE(re.PartialMatch(""));
  ASSERT_FALSE(re.PartialMatch("abcd"));
  ASSERT_FALSE(re.PartialMatch("bcde"));
}

TEST(RegexTest, Choice) {
  pagespeed::RE re;
  ASSERT_TRUE(re.Init(kRegex2));
  ASSERT_TRUE(re.is_valid());

  ASSERT_TRUE(re.PartialMatch("abcde"));
  ASSERT_TRUE(re.PartialMatch("padding abcde padding"));
  ASSERT_TRUE(re.PartialMatch("aaabbbcdde"));

  ASSERT_TRUE(re.PartialMatch("fghij"));
  ASSERT_TRUE(re.PartialMatch("padding fghij padding"));
  ASSERT_TRUE(re.PartialMatch("fffggghiij"));

  ASSERT_FALSE(re.PartialMatch("ZZ-Top Rulz!"));
}

TEST(RegexTest, Invalid) {
  pagespeed::RE re;
  ASSERT_FALSE(re.Init(kRegexInvalid));
  ASSERT_FALSE(re.is_valid());

#ifdef NDEBUG
  ASSERT_FALSE(re.PartialMatch(""));
#else
  ASSERT_DEATH(re.PartialMatch(""), "Check failed: false");
#endif
}

}  // namespace
