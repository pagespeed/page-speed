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

#include "pagespeed/rules/minimize_resources_rule.h"

#include <string>

#include "base/logging.h"
#include "pagespeed/core/pagespeed_input.h"
#include "pagespeed/core/pagespeed_output.pb.h"
#include "pagespeed/core/resource.h"
#include "pagespeed/rules/minimize_resources_details.pb.h"

namespace pagespeed {

MinimizeResourcesRule::MinimizeResourcesRule(const char* rule_name,
                                             ResourceType resource_type)
    : rule_name_(rule_name), resource_type_(resource_type) {
}

bool MinimizeResourcesRule::AppendResults(const PagespeedInput& input,
                                          Results* results) {
  const HostResourceMap& host_resource_map = *input.GetHostResourceMap();

  for (HostResourceMap::const_iterator iter = host_resource_map.begin(),
           end = host_resource_map.end();
       iter != end;
       ++iter) {
    ResourceVector violations;
    for (ResourceVector::const_iterator resource_iter = iter->second.begin(),
             resource_end = iter->second.end();
         resource_iter != resource_end;
         ++resource_iter) {
      const pagespeed::Resource* resource = *resource_iter;

      // exclude non-http resources
      std::string protocol = resource->GetProtocol();
      if (protocol != "http" && protocol != "https") {
        continue;
      }

      if (resource->GetResourceType() != resource_type_) {
        continue;
      }

      const std::string& host = iter->first;
      CHECK(!host.empty());

      violations.push_back(resource);
    }

    if (violations.size() > 1) {
      Result* result = results->add_results();
      result->set_rule_name(rule_name_);

      ResultDetails* details = result->mutable_details();
      pagespeed::MinimizeResourcesDetails* resources_details =
          details->MutableExtension(
              pagespeed::MinimizeResourcesDetails::message_set_extension);

      int requests_saved = 0;
      requests_saved += (violations.size() - 1);
      for (ResourceVector::const_iterator violation_iter = violations.begin(),
               violation_end = violations.end();
           violation_iter != violation_end;
           ++violation_iter) {
        resources_details->add_violation_urls(
          (*violation_iter)->GetRequestUrl());
      }

      resources_details->set_violation_host(iter->first);

      pagespeed::Savings* savings = result->mutable_savings();
      savings->set_requests_saved(requests_saved);
    }
  }

  return true;
}

void MinimizeResourcesRule::InterpretResults(const Results& results,
                                             ResultText* result_text) {
  const char* body_tmpl = NULL;
  if (resource_type_ == CSS) {
    result_text->set_format("Combine external CSS");
    body_tmpl = "There are $1 CSS files served from $2. "
        "They should be combined into as few files as possible.";
  } else if (resource_type_ == JS) {
    result_text->set_format("Combine external Javascript");
    body_tmpl = "There are $1 JavaScript files served from $2. "
        "They should be combined into as few files as possible.";
  } else {
    CHECK(false);
  }

  for (int result_idx = 0; result_idx < results.results_size(); result_idx++) {
    const Result& result = results.results(result_idx);
    const ResultDetails& details = result.details();
    const MinimizeResourcesDetails& minimize_details = details.GetExtension(
        MinimizeResourcesDetails::message_set_extension);

    ResultText* body = result_text->add_children();
    body->set_format(body_tmpl);
    FormatArgument* count = body->add_args();
    count->set_type(FormatArgument::INT_LITERAL);
    count->set_int_literal(minimize_details.violation_urls_size());
    FormatArgument* host = body->add_args();
    host->set_type(FormatArgument::STRING_LITERAL);
    host->set_string_literal(minimize_details.violation_host());

    for (int idx = 0; idx < minimize_details.violation_urls_size(); idx++) {
      ResultText* url = body->add_children();
      url->set_format("$1");

      FormatArgument* url_arg = url->add_args();
      url_arg->set_type(FormatArgument::URL);
      url_arg->set_url(minimize_details.violation_urls(idx));
    }
  }
}

MinimizeJsResourcesRule::MinimizeJsResourcesRule()
    : MinimizeResourcesRule("MinimizeJsResourcesRule", JS) {
}

MinimizeCssResourcesRule::MinimizeCssResourcesRule()
    : MinimizeResourcesRule("MinimizeCssResourcesRule", CSS) {
}

}  // namespace pagespeed
