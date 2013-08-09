// Copyright 2010 Google Inc. All Rights Reserved.
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

#include "pagespeed/rules/enable_keep_alive.h"

#include <string>

#include "base/logging.h"
#include "googleurl/src/gurl.h"
#include "pagespeed/core/formatter.h"
#include "pagespeed/core/image_attributes.h"
#include "pagespeed/core/pagespeed_input.h"
#include "pagespeed/core/resource.h"
#include "pagespeed/core/resource_util.h"
#include "pagespeed/core/result_provider.h"
#include "pagespeed/core/rule_input.h"
#include "pagespeed/core/uri_util.h"
#include "pagespeed/l10n/l10n.h"
#include "pagespeed/proto/pagespeed_output.pb.h"

namespace pagespeed {

namespace rules {

namespace {

const char* kRuleName = "EnableKeepAlive";

}  // namespace

EnableKeepAlive::EnableKeepAlive()
  : pagespeed::Rule(pagespeed::InputCapabilities(
      pagespeed::InputCapabilities::RESPONSE_BODY)) {
}

const char* EnableKeepAlive::name() const {
  return kRuleName;
}

UserFacingString EnableKeepAlive::header() const {
  // TRANSLATOR: The name of a Page Speed rule that tells users to enable
  // keep-alive on their server to save connection time. This is displayed
  // in a list of rule names that Page Speed generates. "Keep-Alive" should not
  // be translated since it is the name of an HTTP header.
  return _("Enable Keep-Alive");
}

bool EnableKeepAlive::AppendResults(const RuleInput& rule_input,
                                 ResultProvider* provider) {
  HostResourceMap violations;
  const PagespeedInput& input = rule_input.pagespeed_input();
  for (int idx = 0, num = input.num_resources(); idx < num; ++idx) {
    const Resource& resource = input.GetResource(idx);

    const std::string& connection = resource.GetResponseHeader("Connection");
    pagespeed::resource_util::DirectiveMap directives;
    if (!pagespeed::resource_util::GetHeaderDirectives(connection,
                                                       &directives)) {
      // Failed to parse directives. Skip the resource.
      continue;
    }

    if (directives.find("keep-alive") != directives.end()) {
      // Keep-Alive is explicitly enabled.
      continue;
    }

    if (resource.GetResponseProtocol() == UNKNOWN_PROTOCOL) {
      // Skip the resouce.
      continue;
    } else if (resource.GetResponseProtocol() == HTTP_11) {
      // Keep-Alive is default in HTTP/1.1. If it is not "close", then
      // Keep-Alive is enabled.
      if (directives.find("close") == directives.end()) {
        continue;
      }
    }

    GURL gurl(resource.GetRequestUrl());
    violations[gurl.host()].insert(&resource);
  }

  const HostResourceMap& host_resource_map = *input.GetHostResourceMap();
  for (HostResourceMap::const_iterator iter = violations.begin(),
       end = violations.end();
       iter != end;
       ++iter) {
    const ResourceSet& resource_set = iter->second;
    // Get all the resources from the violating host. We warn the host if it
    // has more than one resource and has at least one violating resource.
    HostResourceMap::const_iterator host_resource_iter =
        host_resource_map.find(iter->first);
    if (host_resource_iter == host_resource_map.end()) {
      LOG(DFATAL) << "Host not found in host_resource map: " << iter->first;
      continue;
    }

    const ResourceSet& all_resource_set = host_resource_iter->second;
    if (all_resource_set.size() <= 1) {
      // There is no benefit from Keep-Alive if there is only one resource
      // from a host, so don't warn.
      continue;
    }

    Result* result = provider->NewResult();
    int connections_saved;
    if (resource_set.size() == all_resource_set.size()) {
      // If all resources on the host did not enable keep-alive, we
      // can only save that number of resources minus one, since we
      // have to open at least one connection to the host.
      connections_saved = resource_set.size() - 1;
    } else {
      // Otherwise, if some connections to the host did not generate
      // violations, we can save as many connections as there are
      // violating resources, since some other non-violating resource
      // already opened a kept-alive connection.
      connections_saved = resource_set.size();
    }
    for (ResourceSet::const_iterator res_iter = resource_set.begin(),
         res_end = resource_set.end();
         res_iter != res_end;
         ++res_iter) {
      const Resource* resource = *res_iter;
      result->add_resource_urls(resource->GetRequestUrl());
    }
    Savings* savings = result->mutable_savings();
    savings->set_connections_saved(connections_saved);
  }
  return true;
}

void EnableKeepAlive::FormatResults(const ResultVector& results,
                                    RuleFormatter* formatter) {
  if (results.empty()) {
    return;
  }

  for (ResultVector::const_iterator iter = results.begin(),
           end = results.end();
       iter != end;
       ++iter) {
    const Result& result = **iter;
    std::string domain =
        uri_util::GetDomainAndRegistry(result.resource_urls(0));
    UrlBlockFormatter* body = formatter->AddUrlBlock(
        // TRANSLATOR: Header at the top of a list of URLs of resources that
        // are served from a domain that does not have HTTP Keep-Alive enabled.
        // It tells the user to enable keep-alive on that host.  The
        // "HOST_NAME" placeholder gives the name of the host
        // (e.g. "www.example.com").  The "KEEPALIVE" placeholder is replaced
        // with the string "Keep-Alive", which is code that should not be
        // translated.
        _("The host %(HOST_NAME)s should enable %(KEEPALIVE)s. It serves the "
          "following resources."), StringArgument("HOST_NAME", domain),
        StringArgument("KEEPALIVE", "Keep-Alive"));

    for (int idx = 0; idx < result.resource_urls_size(); idx++) {
      body->AddUrl(result.resource_urls(idx));
    }

  }
}

}  // namespace rules

}  // namespace pagespeed
