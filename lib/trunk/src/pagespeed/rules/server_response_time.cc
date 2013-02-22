// Copyright 2013 Google Inc. All Rights Reserved.
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

#include "pagespeed/rules/server_response_time.h"

#include <string>
#include <vector>

#include "base/basictypes.h"
#include "base/logging.h"
#include "pagespeed/core/dom.h"
#include "pagespeed/core/formatter.h"
#include "pagespeed/core/pagespeed_input.h"
#include "pagespeed/core/resource.h"
#include "pagespeed/core/result_provider.h"
#include "pagespeed/core/rule_input.h"
#include "pagespeed/l10n/l10n.h"
#include "pagespeed/proto/pagespeed_output.pb.h"

namespace pagespeed {

namespace rules {

namespace {
  const int32 kFirstByteMillisThreshold = 100;
  const int32 kMillisPerRequestWeight = 100;
}

ServerResponseTime::ServerResponseTime()
    : pagespeed::Rule(pagespeed::InputCapabilities(
        pagespeed::InputCapabilities::FIRST_BYTE_TIMES)) {}

const char* ServerResponseTime::name() const {
  return "ServerResponseTime";
}

UserFacingString ServerResponseTime::header() const {
  // TRANSLATOR: The name of a Page Speed rule that tells users to improve
  // their server response time. This is displayed in a list of
  // rule names that Page Speed generates.
  return _("Improve Server Response Time");
}

bool ServerResponseTime::AppendResults(const RuleInput& rule_input,
                                       ResultProvider* provider) {
  const PagespeedInput& input = rule_input.pagespeed_input();
  for (int i = 0, num = input.num_resources(); i < num; ++i) {
    const Resource& resource = input.GetResource(i);

    if (resource.GetFirstByteMillis() >= kFirstByteMillisThreshold) {
      Result* result = provider->NewResult();
      result->add_resource_urls(resource.GetRequestUrl());
      ResultDetails* details = result->mutable_details();
      ServerResponseTimeDetails* srt_details =
          details->MutableExtension(
              ServerResponseTimeDetails::message_set_extension);

      srt_details->set_first_byte_millis(resource.GetFirstByteMillis());
    }
  }
  return true;
}

void ServerResponseTime::FormatResults(const ResultVector& results,
                                       RuleFormatter* formatter) {
  if (results.empty()) {
    return;
  }

  UrlBlockFormatter* body = formatter->AddUrlBlock(
      // TRANSLATOR: Header at the top of a list of URLs that Page Speed
      // deteced as having slow server response times. It describes the
      // problem.
      _("Long web server response times delay page loading. Reduce your"
        "response times to make your page load faster."));

  for (ResultVector::const_iterator i = results.begin(), end = results.end();
       i != end; ++i) {
    const Result& result = **i;
    if (result.resource_urls_size() != 1) {
      LOG(DFATAL) << "Unexpected number of resource URLs.  Expected 1, Got "
                  << result.resource_urls_size() << ".";
      continue;
    }

    const ResultDetails& details = result.details();
    if (details.HasExtension(
        ServerResponseTimeDetails::message_set_extension)) {
      const ServerResponseTimeDetails& srt_details = details.GetExtension(
          ServerResponseTimeDetails::message_set_extension);

      UserFacingString format_str = not_localized("$1 ($2)");
      body->AddUrlResult(
          format_str, UrlArgument(result.resource_urls(0)),
          DurationArgument(srt_details.first_byte_millis()));
    } else {
      LOG(DFATAL) << "Server Response Time details missing";
    }
  }
}

double ServerResponseTime::ComputeResultImpact(
    const InputInformation& input_info,
    const Result& result) {
  const ServerResponseTimeDetails& srt_details = result.details().GetExtension(
      ServerResponseTimeDetails::message_set_extension);

  const ClientCharacteristics& client = input_info.client_characteristics();
  // Assuming that 100ms roughly equals 1 request and the first 100ms is free:
  return client.requests_weight() *
      (srt_details.first_byte_millis() - kFirstByteMillisThreshold) /
      kMillisPerRequestWeight;
}

}  // namespace rules

}  // namespace pagespeed
