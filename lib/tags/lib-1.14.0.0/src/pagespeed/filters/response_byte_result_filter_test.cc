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

#include "pagespeed/filters/response_byte_result_filter.h"
#include "pagespeed/proto/pagespeed_output.pb.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace pagespeed {

TEST(ResponseByteResultFilterTest, ResponseByteResultFilter) {
  Result result;
  ResponseByteResultFilter filter(10);

  EXPECT_TRUE(filter.IsAccepted(result));

  Savings* savings = result.mutable_savings();
  EXPECT_TRUE(filter.IsAccepted(result));

  savings->set_response_bytes_saved(100);
  EXPECT_TRUE(filter.IsAccepted(result));

  savings->set_response_bytes_saved(10);
  EXPECT_TRUE(filter.IsAccepted(result));

  savings->set_response_bytes_saved(9);
  EXPECT_FALSE(filter.IsAccepted(result));

  savings->set_response_bytes_saved(0);
  EXPECT_FALSE(filter.IsAccepted(result));
}

TEST(ResponseByteResultFilterTest, DefaultThreshold) {
  Result result;
  ResponseByteResultFilter filter;

  Savings* savings = result.mutable_savings();
  savings->set_response_bytes_saved(
      ResponseByteResultFilter::kDefaultThresholdBytes);
  EXPECT_TRUE(filter.IsAccepted(result));

  savings->set_response_bytes_saved(
      ResponseByteResultFilter::kDefaultThresholdBytes - 1);
  EXPECT_FALSE(filter.IsAccepted(result));
}

}  // namespace pagespeed
