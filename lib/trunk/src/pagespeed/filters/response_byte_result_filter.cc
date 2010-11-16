// Copyright 2010 Google Inc.
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

namespace pagespeed {

ResponseByteResultFilter::ResponseByteResultFilter(int threshold)
    : response_byte_threshold_(threshold) {
}

ResponseByteResultFilter::ResponseByteResultFilter()
    : response_byte_threshold_(kDefaultThresholdBytes) {
}

ResponseByteResultFilter::~ResponseByteResultFilter() {
}

bool ResponseByteResultFilter::IsAccepted(const Result& result) const {
  if (!result.has_savings()) {
    return true;
  }

  const Savings& savings = result.savings();
  if (!savings.has_response_bytes_saved()) {
    return true;
  }

  return savings.response_bytes_saved() >= response_byte_threshold_;
}

}
