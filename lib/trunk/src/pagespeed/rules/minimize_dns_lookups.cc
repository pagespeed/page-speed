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
#include "build/build_config.h"
#include "pagespeed/core/formatter.h"
#include "pagespeed/core/pagespeed_input.h"
#include "pagespeed/core/resource.h"
#include "pagespeed/core/result_provider.h"
#include "pagespeed/proto/pagespeed_output.pb.h"

// Unfortunately, including Winsock2.h before the other dependencies
// causes a build error due to a conflict with the protobuf library,
// so we are forced to push these includes to the bottom of the
// include list. See
// http://code.google.com/p/protobuf/issues/detail?id=44 for more
// information.
#if defined(OS_WIN)
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

MinimizeDnsLookups::MinimizeDnsLookups() {
}

const char* MinimizeDnsLookups::name() const {
  return "MinimizeDnsLookups";
}

const char* MinimizeDnsLookups::header() const {
  return "Minimize DNS lookups";
}

const char* MinimizeDnsLookups::documentation_url() const {
  return "rtt.html#MinimizeDNSLookups";
}

bool MinimizeDnsLookups::AppendResults(const PagespeedInput& input,
                                       ResultProvider* provider) {
  // TODO This logic is wrong.  Right now, if you have two resources in
  //      different domains, this rule will complain about both resources, and
  //      claim a savings of one DNS request for each resource.  However, you
  //      can't actually save two DNS requests by combining the resources --
  //      you can only save one.

  const HostResourceMap& host_resource_map = *input.GetHostResourceMap();

  // Find resources that are the only (http, non-lazy-loaded) resource from
  // their domain.
  ResourceVector lone_dns_resources;
  for (HostResourceMap::const_iterator iter = host_resource_map.begin(),
           end = host_resource_map.end();
       iter != end;
       ++iter) {
    const std::string& host = iter->first;

    // If an ip address appears explicitly, no DNS lookup is required.
    if (IsAnIPAddress(host.c_str())) {
      continue;
    }

    const ResourceVector& resources = iter->second;

    ResourceVector filtered;
    for (ResourceVector::const_iterator resource_iter = resources.begin(),
             resource_end = resources.end();
         resource_iter != resource_end; ++resource_iter) {
      const Resource* resource = *resource_iter;

      // exclude non-http resources
      const std::string& protocol = resource->GetProtocol();
      if (protocol != "http" && protocol != "https") {
        continue;
      }

      // exclude lazy-loaded resources
      if (resource->IsLazyLoaded()) {
        continue;
      }

      if (host.empty()) {
        LOG(DFATAL) << "Empty host while processing "
                    << resource->GetRequestUrl();
        continue;
      }
      filtered.push_back(resource);
    }

    if (filtered.size() != 1) {
      continue;
    }

    if (filtered[0]->GetRequestUrl() == input.primary_resource_url()) {
      // Special case: if the hostname serves only the main resource,
      // we should not flag that hostname.
      continue;
    }

    lone_dns_resources.push_back(*filtered.begin());
  }

  // Report a result for each lone resource.
  for (ResourceVector::const_iterator iter = lone_dns_resources.begin(),
           end = lone_dns_resources.end();
       iter != end; ++iter) {
    const Resource* resource = *iter;

    Result* result = provider->NewResult();
    result->add_resource_urls(resource->GetRequestUrl());

    Savings* savings = result->mutable_savings();
    savings->set_dns_requests_saved(1);
  }

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

  Formatter* body = formatter->AddChild(
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
