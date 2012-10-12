// Copyright 2011 Google Inc. All Rights Reserved.
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

#include "pagespeed/core/formatter.h"
#include "pagespeed/proto/pagespeed_proto_formatter.pb.h"
#include "testing/gtest/include/gtest/gtest.h"

using pagespeed::PercentageArgument;

namespace {

TEST(FormatterTest, PercentageArgument) {
  EXPECT_EQ(100, PercentageArgument(1, 1).int_value());
  EXPECT_EQ(99, PercentageArgument(995, 1000).int_value());
  EXPECT_EQ(50, PercentageArgument(1, 2).int_value());
  EXPECT_EQ(25, PercentageArgument(10, 40).int_value());
  EXPECT_EQ(1, PercentageArgument(5, 1000).int_value());
  EXPECT_EQ(0, PercentageArgument(0, 1000).int_value());
  EXPECT_EQ(0, PercentageArgument(0, 0).int_value());
}

}  // namespace
