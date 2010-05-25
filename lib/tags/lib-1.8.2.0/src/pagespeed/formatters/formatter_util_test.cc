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

#include "pagespeed/formatters/formatter_util.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

TEST(FormatterUtilTest, Basic) {
  ASSERT_EQ("0 seconds", pagespeed::formatters::FormatTimeDuration(0LL));
  ASSERT_EQ("10 milliseconds", pagespeed::formatters::FormatTimeDuration(10LL));
  ASSERT_EQ("20 seconds", pagespeed::formatters::FormatTimeDuration(20000LL));
  ASSERT_EQ("1 minute", pagespeed::formatters::FormatTimeDuration(60000LL));
  ASSERT_EQ("5 minutes", pagespeed::formatters::FormatTimeDuration(300000LL));
  ASSERT_EQ("2 hours", pagespeed::formatters::FormatTimeDuration(7200000LL));
  ASSERT_EQ("3 days", pagespeed::formatters::FormatTimeDuration(259200000LL));
  ASSERT_EQ("30 days", pagespeed::formatters::FormatTimeDuration(2592000000LL));
  ASSERT_EQ("100 days",
            pagespeed::formatters::FormatTimeDuration(8640000000LL));
  ASSERT_EQ("1 year",
            pagespeed::formatters::FormatTimeDuration(31536000000LL));
  ASSERT_EQ("10 years",
            pagespeed::formatters::FormatTimeDuration(315360000000LL));
}

TEST(FormatterUtilTest, Multi) {
  ASSERT_EQ("20 seconds 20 milliseconds",
            pagespeed::formatters::FormatTimeDuration(20020LL));
  ASSERT_EQ("1 minute 10 seconds",
            pagespeed::formatters::FormatTimeDuration(70000LL));
  ASSERT_EQ("1 minute 10 seconds",
            pagespeed::formatters::FormatTimeDuration(70100LL));
  ASSERT_EQ("30 days 5 minutes",
            pagespeed::formatters::FormatTimeDuration(2592300000LL));
  ASSERT_EQ("30 days 5 minutes",
            pagespeed::formatters::FormatTimeDuration(2592300040LL));
  ASSERT_EQ("30 days 1 millisecond",
            pagespeed::formatters::FormatTimeDuration(2592000001LL));
  ASSERT_EQ("1 year 10 milliseconds",
            pagespeed::formatters::FormatTimeDuration(31536000010LL));
}

}
