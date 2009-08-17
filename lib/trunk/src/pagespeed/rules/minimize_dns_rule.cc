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

#include "pagespeed/rules/minimize_dns_rule.h"

#include <arpa/inet.h>
#include <string>

#include "base/logging.h"
#include "pagespeed/core/pagespeed_input.h"
#include "pagespeed/core/resource.h"
#include "pagespeed/core/pagespeed_output.pb.h"
#include "pagespeed/rules/minimize_dns_details.pb.h"

namespace {

bool IsAnIPAddress(const char* str) {
  if (!str) {
    return false;
  }

  // Try to parse the string as an IPv4 address first. (glibc does not
  // yet recognize IPv4 addresses when given an address family of
  // AF_INET, but at some point it will, and at that point the other way
  // around would break.)
  in_addr addr4;
  if (inet_pton(AF_INET, str, &addr4) > 0) {
    return true;
  }

  in6_addr addr6;
  if (inet_pton(AF_INET6, str, &addr6) > 0) {
    return true;
  }

  return false;
}

}  // namespace

namespace pagespeed {

MinimizeDnsRule::MinimizeDnsRule() {
}

bool MinimizeDnsRule::AppendResults(const PagespeedInput& input,
                                    Results* results) {
  Result* result = results->add_results();
  result->set_rule_name("MinimizeDnsRule");

  const HostResourceMap& host_resource_map = *input.GetHostResourceMap();

  ResultDetails* details = result->mutable_details();
  MinimizeDnsDetails* dns_details = details->MutableExtension(
      MinimizeDnsDetails::message_set_extension);
      dns_details->set_num_hosts(host_resource_map.size());

  int dns_requests_saved = 0;

  // Only check if resources are sharded among 2 or more hosts.  We
  // should not warn about simple pages that consist of a single
  // resource since in order to access a page, we must always perform
  // at least 1 dns lookup.
  if (host_resource_map.size() > 1) {
    for (HostResourceMap::const_iterator iter = host_resource_map.begin(),
             end = host_resource_map.end();
         iter != end;
         ++iter) {
      const std::string& host = iter->first;

      ResourceVector filtered;
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

        CHECK(!host.empty());
        filtered.push_back(resource);
      }

      if (filtered.size() == 1) {
        // If the ip address is appears explicitly, no DNS lookup is required.
        bool need_dns = !IsAnIPAddress(host.c_str());
        if (!need_dns) {
          continue;
        }

        dns_requests_saved++;
        for (ResourceVector::const_iterator resource_iter = filtered.begin(),
                 url_end = filtered.end();
             resource_iter != url_end;
             ++resource_iter) {
          dns_details->add_violation_urls((*resource_iter)->GetRequestUrl());
        }
      }
    }
  }

  Savings* savings = result->mutable_savings();
  savings->set_dns_requests_saved(dns_requests_saved);

  return true;
}

}  // namespace pagespeed
