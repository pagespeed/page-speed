// Copyright 2012 Google Inc.
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

namespace pagespeed {

LandingPageRedirectionFilter::LandingPageRedirectionFilter(int threshold)
    : redirection_count_threshold_(threshold) {
}

LandingPageRedirectionFilter::LandingPageRedirectionFilter()
    : redirection_count_threshold_(kDefaultThresholdRedirectionCount) {
}

LandingPageRedirectionFilter::~LandingPageRedirectionFilter() {
}

bool LandingPageRedirectionFilter::IsAccepted(const Result& result) const {
  if (!result.has_savings()) {
    return true;
  }

  const Savings& savings = result.savings();
  if (!savings.has_requests_saved()) {
    return true;
  }

  const ResultDetails& result_details = result.details();
  if (!result_details.HasExtension(RedirectionDetails::message_set_extension)) {
    return true;
  }

  const RedirectionDetails& details =
      result_details.GetExtension(RedirectionDetails::message_set_extension);
  if (details.chain_length() <= redirection_count_threshold_) {
    if (details.is_cacheable() && !details.is_same_host()) {
      return false;
    } else if (details.is_likely_login() || details.is_likely_callback()) {
      return false;
    }
  }
  return true;
}

}  // namespace pagespeed
