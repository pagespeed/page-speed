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

#include "pagespeed/filters/landing_page_redirection_filter.h"
#include "pagespeed/proto/pagespeed_output.pb.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace pagespeed {

TEST(LandingPageRedirectionFilterTest, LandingPageRedirectionFilter) {
  Result result;
  LandingPageRedirectionFilter filter;

  EXPECT_TRUE(filter.IsAccepted(result));

  Savings* savings = result.mutable_savings();
  EXPECT_TRUE(filter.IsAccepted(result));

  savings->set_requests_saved(1);
  EXPECT_TRUE(filter.IsAccepted(result));

  ResultDetails* result_details = result.mutable_details();
  RedirectionDetails* details = result_details->MutableExtension(
      RedirectionDetails::message_set_extension);

  details->set_chain_length(
      LandingPageRedirectionFilter::kDefaultThresholdRedirectionCount);

  // Permanent, but not cacheable.
  details->set_is_permanent(true);
  EXPECT_TRUE(filter.IsAccepted(result));

  // Not permanent, not cacheable.
  details->set_is_permanent(false);
  EXPECT_TRUE(filter.IsAccepted(result));

  // Not cacheable.
  details->set_is_cacheable(false);
  EXPECT_TRUE(filter.IsAccepted(result));

  // Not cacheable, and same host.
  details->set_is_same_host(true);
  EXPECT_TRUE(filter.IsAccepted(result));

  // Cacheable, and not same host.
  details->set_is_cacheable(true);
  details->set_is_same_host(false);
  EXPECT_FALSE(filter.IsAccepted(result));

  // Not cacheable, and login.
  details->set_is_cacheable(false);
  details->set_is_likely_login(true);
  EXPECT_FALSE(filter.IsAccepted(result));

  // Cacheable, and login.
  details->set_is_cacheable(true);
  EXPECT_FALSE(filter.IsAccepted(result));

  // Cacheable, and likely callback, but not login.
  details->set_is_likely_login(false);
  details->set_is_likely_callback(true);
  EXPECT_FALSE(filter.IsAccepted(result));

  // Not cacheable, and likely callback, but not login.
  details->set_is_cacheable(false);
  EXPECT_FALSE(filter.IsAccepted(result));


  // Cacheable with same host, but neitehr likely callback, nor login.
  details->set_is_same_host(true);
  details->set_is_likely_callback(false);
  EXPECT_TRUE(filter.IsAccepted(result));
}

TEST(LandingPageRedirectionFilterTest, DefaultThreshold) {
  Result result;
  LandingPageRedirectionFilter filter(
      LandingPageRedirectionFilter::kDefaultThresholdRedirectionCount - 1);

  Savings* savings = result.mutable_savings();
  EXPECT_TRUE(filter.IsAccepted(result));

  savings->set_requests_saved(1);
  EXPECT_TRUE(filter.IsAccepted(result));

  ResultDetails* result_details = result.mutable_details();
  RedirectionDetails* details = result_details->MutableExtension(
      RedirectionDetails::message_set_extension);

  details->set_chain_length(
      LandingPageRedirectionFilter::kDefaultThresholdRedirectionCount);

  // Redirection chain is longer than threshold, we still accept the result with
  // cacheable and non-same host redirections.
  details->set_chain_length(
      LandingPageRedirectionFilter::kDefaultThresholdRedirectionCount);
  details->set_is_cacheable(true);
  details->set_is_same_host(false);
  EXPECT_TRUE(filter.IsAccepted(result));
}

}  // namespace pagespeed
