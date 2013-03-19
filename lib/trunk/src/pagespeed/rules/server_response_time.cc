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
#include "pagespeed/proto/resource.pb.h"

namespace pagespeed {

namespace rules {

namespace {
  // TODO(mankoff): These weights are quick hack to get around the fact that we
  // don't track what is loaded syncrhonously vs asynchronously currently.
  //Instead we simply assign HTML a higher weight than other resources.
  const int32 kFirstByteMillisThreshold = 100;
  const int32 kMillisPerRequestWeight = 100;
  const double kHtmlWeight = 1.0;
  const double kCssWeight = 0.5;
  const double kJsWeight = 0.5;
  const double kOtherWeight = 0.1;
  // TODO(mankoff): Break out these weights further? Images, Flash, Iframes vs
  // main HTML, etc.
}

ServerResponseTime::ServerResponseTime()
    : pagespeed::Rule(pagespeed::InputCapabilities(
        pagespeed::InputCapabilities::FIRST_BYTE_TIMES |
        pagespeed::InputCapabilities::ONLOAD |
        pagespeed::InputCapabilities::REQUEST_START_TIMES)) {}

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
  const RedirectRegistry* redirect_registry =
      input.GetResourceCollection().GetRedirectRegistry();

  for (int i = 0, num = input.num_resources(); i < num; ++i) {
    const Resource& resource = input.GetResource(i);
    if (input.IsResourceLoadedAfterOnload(resource)) {
      continue;
    }

    if (resource.GetFirstByteMillis() >= kFirstByteMillisThreshold) {
      const Resource* final_resource =
          redirect_registry->GetFinalRedirectTarget(&resource);
      Result* result = provider->NewResult();
      result->add_resource_urls(resource.GetRequestUrl());
      ResultDetails* details = result->mutable_details();
      ServerResponseTimeDetails* srt_details =
          details->MutableExtension(
              ServerResponseTimeDetails::message_set_extension);

      srt_details->set_first_byte_millis(resource.GetFirstByteMillis());
      srt_details->set_resource_type(resource.GetResourceType());
      if (final_resource != NULL && final_resource != &resource) {
        srt_details->set_final_resource_type(
            final_resource->GetResourceType());
      }
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
      // detected as having slow server response times. It describes the
      // problem.
      _("Long web server response times delay page loading. Reduce your "
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

  const ResourceType resource_type = srt_details.has_final_resource_type() ?
      srt_details.final_resource_type() : srt_details.resource_type();

  double weight = kOtherWeight;
  if (resource_type == HTML) {
    weight = kHtmlWeight;
  } else if (resource_type == JS) {
    weight = kJsWeight;
  } else if (resource_type == CSS) {
    weight = kCssWeight;
  }

  return client.requests_weight() * weight *
      (srt_details.first_byte_millis() - kFirstByteMillisThreshold) /
      kMillisPerRequestWeight;
}

}  // namespace rules

}  // namespace pagespeed
