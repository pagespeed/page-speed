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

#include "pagespeed/rules/minimize_dns_lookups.h"

#include <string>
#include <vector>

#include "base/logging.h"
#include "pagespeed/core/formatter.h"
#include "pagespeed/core/pagespeed_input.h"
#include "pagespeed/core/resource.h"
#include "pagespeed/proto/pagespeed_output.pb.h"

// Unfortunately, including Winsock2.h before the other dependencies
// causes a build error due to a conflict with the protobuf library,
// so we are forced to push these includes to the bottom of the
// include list. See
// http://code.google.com/p/protobuf/issues/detail?id=44 for more
// information.
#if defined(_WINDOWS)
#include <Winsock2.h>
#else
#include <arpa/inet.h>
#endif

namespace {

bool IsAnIPAddress(const char* str) {
  if (!str) {
    return false;
  }

  if (inet_addr(str) != INADDR_NONE) {
    return true;
  }

  // TODO: add support for IPv6 addresses. See
  // http://code.google.com/p/page-speed/issues/detail?id=109 for more
  // information.

  return false;
}

}  // namespace

namespace pagespeed {

namespace rules {

MinimizeDnsLookups::MinimizeDnsLookups() : Rule("MinimizeDnsLookups") {
}

bool MinimizeDnsLookups::AppendResults(const PagespeedInput& input,
                                       Results* results) {
  Result* result = results->add_results();
  result->set_rule_name(name());

  const HostResourceMap& host_resource_map = *input.GetHostResourceMap();

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
          result->add_resource_urls((*resource_iter)->GetRequestUrl());
        }
      }
    }
  }

  Savings* savings = result->mutable_savings();
  savings->set_dns_requests_saved(dns_requests_saved);

  return true;
}

void MinimizeDnsLookups::FormatResults(const ResultVector& results,
                                       Formatter* formatter) {
  std::vector<std::string> violation_urls;
  for (ResultVector::const_iterator iter = results.begin(),
           end = results.end();
       iter != end;
       ++iter) {
    const Result& result = **iter;

    for (int idx = 0; idx < result.resource_urls_size(); idx++) {
      violation_urls.push_back(result.resource_urls(idx));
    }
  }

  if (violation_urls.empty()) {
    return;
  }

  Formatter* header = formatter->AddChild("Minimize DNS lookups");

  Formatter* body = header->AddChild(
      "The domains of the following urls only serve one "
      "resource each. If possible, avoid the extra DNS "
      "lookups by serving these resources from existing domains.");

  for (std::vector<std::string>::const_iterator iter = violation_urls.begin(),
           end = violation_urls.end();
       iter != end;
       ++iter) {
    Argument url(Argument::URL, *iter);
    body->AddChild("$1", url);
  }
}

}  // namespace rules

}  // namespace pagespeed
