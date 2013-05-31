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

#include "pagespeed/core/pagespeed_input_util.h"

#include "pagespeed/proto/pagespeed_output.pb.h"

namespace pagespeed {

namespace pagespeed_input_util {

void PopulateMobileClientCharacteristics(
    pagespeed::ClientCharacteristics* client_characteristics) {
  // Clear the proto, which resets all its fields to their default values,
  // which are the values for desktop clients.  We can then apply the
  // multipliers below for certain fields to get the values we want for mobile
  // clients.
  client_characteristics->Clear();

  // 5x the desktop value
  client_characteristics->set_javascript_parse_weight(
      5.0 * client_characteristics->javascript_parse_weight());

  // 1.5x the desktop value
  client_characteristics->set_dns_requests_weight(
      1.5 * client_characteristics->dns_requests_weight());
  client_characteristics->set_requests_weight(
      1.5 * client_characteristics->requests_weight());

  // 4x the desktop value
  client_characteristics->set_render_blocking_round_trips_weight(
      4.0 * client_characteristics->render_blocking_round_trips_weight());
}

}  // namespace pagespeed_input_util

}  // namespace pagespeed
