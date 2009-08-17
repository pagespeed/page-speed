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
  Result* result = results->add_results();
  result->set_rule_name(rule_name_);

  const HostResourceMap& host_resource_map = *input.GetHostResourceMap();

  ResultDetails* details = result->mutable_details();
  pagespeed::MinimizeResourcesDetails* resources_details =
      details->MutableExtension(
      pagespeed::MinimizeResourcesDetails::message_set_extension);

  int requests_saved = 0;
  int num_violation_hosts = 0;

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
      num_violation_hosts++;
      requests_saved += (violations.size() - 1);
      for (ResourceVector::const_iterator violation_iter = violations.begin(),
               violation_end = violations.end();
           violation_iter != violation_end;
           ++violation_iter) {
        resources_details->add_violation_urls(
          (*violation_iter)->GetRequestUrl());
      }
    }
  }

  resources_details->set_num_violation_hosts(num_violation_hosts);

  pagespeed::Savings* savings = result->mutable_savings();
  savings->set_requests_saved(requests_saved);

  return true;
}

MinimizeJsResourcesRule::MinimizeJsResourcesRule()
    : MinimizeResourcesRule("MinimizeJsResourcesRule", JS) {
}

MinimizeCssResourcesRule::MinimizeCssResourcesRule()
    : MinimizeResourcesRule("MinimizeCssResourcesRule", CSS) {
}

}  // namespace pagespeed
