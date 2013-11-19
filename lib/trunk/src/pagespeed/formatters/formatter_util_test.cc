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
  EXPECT_EQ("0 seconds", pagespeed::formatters::FormatTimeDuration(0LL));
  EXPECT_EQ("10 milliseconds", pagespeed::formatters::FormatTimeDuration(10LL));
  EXPECT_EQ("20 seconds", pagespeed::formatters::FormatTimeDuration(20000LL));
  EXPECT_EQ("1 minute", pagespeed::formatters::FormatTimeDuration(60000LL));
  EXPECT_EQ("5 minutes", pagespeed::formatters::FormatTimeDuration(300000LL));
  EXPECT_EQ("2 hours", pagespeed::formatters::FormatTimeDuration(7200000LL));
  EXPECT_EQ("3 days", pagespeed::formatters::FormatTimeDuration(259200000LL));
  EXPECT_EQ("30 days", pagespeed::formatters::FormatTimeDuration(2592000000LL));
  EXPECT_EQ("100 days",
            pagespeed::formatters::FormatTimeDuration(8640000000LL));
  EXPECT_EQ("1 year",
            pagespeed::formatters::FormatTimeDuration(31536000000LL));
  EXPECT_EQ("10 years",
            pagespeed::formatters::FormatTimeDuration(315360000000LL));
}

TEST(FormatterUtilTest, Multi) {
  EXPECT_EQ("20 seconds 20 milliseconds",
            pagespeed::formatters::FormatTimeDuration(20020LL));
  EXPECT_EQ("1 minute 10 seconds",
            pagespeed::formatters::FormatTimeDuration(70000LL));
  EXPECT_EQ("1 minute 10 seconds",
            pagespeed::formatters::FormatTimeDuration(70100LL));
  EXPECT_EQ("30 days 5 minutes",
            pagespeed::formatters::FormatTimeDuration(2592300000LL));
  EXPECT_EQ("30 days 5 minutes",
            pagespeed::formatters::FormatTimeDuration(2592300040LL));
  EXPECT_EQ("30 days 1 millisecond",
            pagespeed::formatters::FormatTimeDuration(2592000001LL));
  EXPECT_EQ("1 year 10 milliseconds",
            pagespeed::formatters::FormatTimeDuration(31536000010LL));
}

TEST(FormatterUtilTest, Distance) {
  EXPECT_EQ("0mm", pagespeed::formatters::FormatDistance(-10));
  EXPECT_EQ("0mm", pagespeed::formatters::FormatDistance(0));
  EXPECT_EQ("0.01mm", pagespeed::formatters::FormatDistance(1));
  EXPECT_EQ("0.1mm", pagespeed::formatters::FormatDistance(100));
  EXPECT_EQ("0.12mm", pagespeed::formatters::FormatDistance(123));
  EXPECT_EQ("0.13mm", pagespeed::formatters::FormatDistance(126));
  EXPECT_EQ("1mm", pagespeed::formatters::FormatDistance(999));
  EXPECT_EQ("1mm", pagespeed::formatters::FormatDistance(1000));
  EXPECT_EQ("4.6mm", pagespeed::formatters::FormatDistance(4567));
  EXPECT_EQ("3.9mm", pagespeed::formatters::FormatDistance(3949));
  EXPECT_EQ("4mm", pagespeed::formatters::FormatDistance(3950));
  EXPECT_EQ("4mm", pagespeed::formatters::FormatDistance(4049));
  EXPECT_EQ("4.1mm", pagespeed::formatters::FormatDistance(4050));
  EXPECT_EQ("25mm", pagespeed::formatters::FormatDistance(24680));
  EXPECT_EQ("314mm", pagespeed::formatters::FormatDistance(314159));
  EXPECT_EQ("1m", pagespeed::formatters::FormatDistance(999949));
  EXPECT_EQ("1m", pagespeed::formatters::FormatDistance(999950));
  EXPECT_EQ("1m", pagespeed::formatters::FormatDistance(999999));
  EXPECT_EQ("1m", pagespeed::formatters::FormatDistance(1000000));
  EXPECT_EQ("1m", pagespeed::formatters::FormatDistance(1049999));
  EXPECT_EQ("1.1m", pagespeed::formatters::FormatDistance(1050000));
  EXPECT_EQ("1.1m", pagespeed::formatters::FormatDistance(1100000));
  EXPECT_EQ("8.7m", pagespeed::formatters::FormatDistance(8675309));
  EXPECT_EQ("27183km", pagespeed::formatters::FormatDistance(27182818284590));
}

}
