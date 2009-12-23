// Copyright 2009 Google Inc. All Rights Reserved.
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

#include "pagespeed/core/rule.h"

#include <algorithm>
#include "pagespeed/proto/pagespeed_output.pb.h"

namespace {

// Scoring algorithm constants.
// Actual values picked such that the algorithm below produces results
// that are similar to those produced by the algorithm used in the
// pagespeed firefox extension for several sample pages.  Beyond that,
// the request bytes impact was picked to maximize dynamic range,
// request impact should be higher than request bytes impact since it
// adds round trips, and DNS lookup impact should be higher than
// request impact since pending DNS lookups block regular requests.
// Expect these constants and/or the algorithm below to change as we learn
// more about how it performs against a larger set of pages and we start
// trying to draw correlations between page load times and scores.
const double kRequestBytesImpact = 3.0;
const double kRequestImpact = 5.0;
const double kDnsLookupImpact = 1.5 * kRequestImpact;

}

namespace pagespeed {

Rule::Rule() {}

Rule::~Rule() {}

int Rule::ComputeScore(const InputInformation& input_info,
                       const ResultVector& results) {
  int bytes_saved = 0, dns_saved = 0, requests_saved = 0;
  for (std::vector<const Result*>::const_iterator iter = results.begin(),
           end = results.end();
       iter != end;
       ++iter) {
    const Result* result = *iter;
    if (result->has_savings()) {
      const Savings& savings = result->savings();
      bytes_saved += savings.response_bytes_saved();
      dns_saved += savings.dns_requests_saved();
      requests_saved += savings.requests_saved();
    }
  }

  // TODO improve this scoring heuristic
  double normalized_savings = 0;
  if (bytes_saved > 0) {
    if (input_info.total_response_bytes() == 0) {
      return -1;  // information is not available
    }
    normalized_savings +=
        kRequestBytesImpact * bytes_saved / input_info.total_response_bytes();
  }

  if (dns_saved > 0) {
    if (input_info.number_resources() == 0) {
      return -1;  // information is not available
    }
    normalized_savings +=
        kDnsLookupImpact * dns_saved / input_info.number_resources();
  }

  if (requests_saved > 0) {
    if (input_info.number_resources() == 0) {
      return -1;  // information is not available
    }
    normalized_savings +=
        kRequestImpact * requests_saved / input_info.number_resources();
  }

  return std::max(0, (int)(100 * (1.0 - normalized_savings)));
}

}  // namespace pagespeed
