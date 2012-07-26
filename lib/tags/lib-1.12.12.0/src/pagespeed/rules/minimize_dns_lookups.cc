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

#include <map>
#include <string>
#include <vector>

#include "base/logging.h"
#include "build/build_config.h"
#include "googleurl/src/gurl.h"
#include "pagespeed/core/formatter.h"
#include "pagespeed/core/pagespeed_input.h"
#include "pagespeed/core/resource.h"
#include "pagespeed/core/result_provider.h"
#include "pagespeed/core/rule_input.h"
#include "pagespeed/core/uri_util.h"
#include "pagespeed/l10n/l10n.h"
#include "pagespeed/proto/pagespeed_output.pb.h"

namespace {
// We build a map from domain (e.g. example.com) to hostname
// (e.g. a.b.example.com) to Resources
// (e.g. http://a.b.example.com/example.css). This map allows us to
// identify resources that are candidates for moving to other domains,
// which can reduce the number of DNS lookups.
typedef std::map<std::string, pagespeed::HostResourceMap> DomainHostResourceMap;

void PopulateDomainHostResourceMap(
    const pagespeed::PagespeedInput& input,
    DomainHostResourceMap *domain_host_resouce_map) {
  for (int i = 0, num = input.num_resources(); i < num; ++i) {
    const pagespeed::Resource& resource = input.GetResource(i);
    // exclude non-http resources
    const std::string& protocol = resource.GetProtocol();
    if (protocol != "http" && protocol != "https") {
      continue;
    }

    // exclude post-onload resources
    if (input.IsResourceLoadedAfterOnload(resource)) {
      continue;
    }

    std::string domain =
        pagespeed::uri_util::GetDomainAndRegistry(resource.GetRequestUrl());
    if (domain.empty()) {
      LOG(INFO) << "Got empty domain for " << resource.GetRequestUrl();
      continue;
    }

    // Add the resource to the map.
    GURL gurl(resource.GetRequestUrl());
    (*domain_host_resouce_map)[domain][gurl.host()].insert(&resource);
  }
}

void PopulateLoneDnsResources(
    const pagespeed::RuleInput& rule_input,
    const pagespeed::HostResourceMap &host_resource_map,
    pagespeed::ResourceSet *lone_dns_resources) {
  const pagespeed::PagespeedInput& input = rule_input.pagespeed_input();
  std::string primary_resource_url;
  if (!pagespeed::uri_util::GetUriWithoutFragment(input.primary_resource_url(),
                                                  &primary_resource_url)) {
    primary_resource_url = input.primary_resource_url();
  }
  const pagespeed::Resource* primary_resource =
      input.GetResourceWithUrlOrNull(primary_resource_url);
  const pagespeed::RedirectRegistry::RedirectChain* primary_resource_chain =
      input.GetResourceCollection()
      .GetRedirectRegistry()->GetRedirectChainOrNull(primary_resource);
  for (pagespeed::HostResourceMap::const_iterator iter =
           host_resource_map.begin(), end = host_resource_map.end();
       iter != end;
       ++iter) {
    const pagespeed::ResourceSet& resources = iter->second;
    DCHECK(!resources.empty());
    if (resources.size() != 1) {
      // If there's more than one resource, then it's not a candidate
      // for a lone DNS lookup.
      continue;
    }
    const pagespeed::Resource* resource = *resources.begin();
    if (resource == primary_resource) {
      // Special case: if this resource is the primary resource, don't
      // flag it since it's not realistic for the site to change the
      // URL of the primary resource.
      continue;
    }
    const pagespeed::RedirectRegistry::RedirectChain* resource_chain =
        input.GetResourceCollection()
        .GetRedirectRegistry()->GetRedirectChainOrNull(resource);
    if (resource_chain != NULL && resource_chain == primary_resource_chain) {
      // Special case: if this resource is a redirect to the primary
      // resource, don't flag it since it's not realistic for the site
      // to change the URL of the primary resource.
      continue;
    }
    lone_dns_resources->insert(resource);
  }
}

void AppendResult(const pagespeed::ResourceSet &lone_dns_resources,
                  bool additional_hostname_available,
                  pagespeed::ResultProvider *provider) {
  pagespeed::Result* result = provider->NewResult();
  for (pagespeed::ResourceSet::const_iterator iter =
           lone_dns_resources.begin(), end = lone_dns_resources.end();
       iter != end; ++iter) {
    const pagespeed::Resource* resource = *iter;
    result->add_resource_urls(resource->GetRequestUrl());
  }

  int num_dns_requests_saved = lone_dns_resources.size();
  if (!additional_hostname_available) {
    // Special case: every hostname on the domain had a single
    // resource. So combining them will still require one domain. Thus
    // we save one fewer DNS requests than the number of lone DNS
    // resources.
    --num_dns_requests_saved;
  }
  pagespeed::Savings* savings = result->mutable_savings();
  savings->set_dns_requests_saved(num_dns_requests_saved);
}

}  // namespace

namespace pagespeed {

namespace rules {

MinimizeDnsLookups::MinimizeDnsLookups()
    : pagespeed::Rule(pagespeed::InputCapabilities(
        pagespeed::InputCapabilities::ONLOAD |
        pagespeed::InputCapabilities::REQUEST_START_TIMES)) {
}

const char* MinimizeDnsLookups::name() const {
  return "MinimizeDnsLookups";
}

UserFacingString MinimizeDnsLookups::header() const {
  // TRANSLATOR: Name of a Page Speed rule. A "DNS lookup" is a
  // request that the browser issues to resolve a hostname to an
  // internet address. The word "DNS" should remain in the translated
  // string.
  return _("Minimize DNS lookups");
}

bool MinimizeDnsLookups::AppendResults(const RuleInput& rule_input,
                                       ResultProvider* provider) {
  const PagespeedInput& input = rule_input.pagespeed_input();
  DomainHostResourceMap domain_host_resouce_map;
  PopulateDomainHostResourceMap(input, &domain_host_resouce_map);

  for (DomainHostResourceMap::const_iterator iter =
           domain_host_resouce_map.begin(), end = domain_host_resouce_map.end();
       iter != end;
       ++iter) {
    const HostResourceMap& host_resource_map = iter->second;
    if (host_resource_map.size() <= 1) {
      // If there's only a single hostname for this domain, it's not
      // realistic to expect the site to re-host resources from a
      // domain they don't control on a different domain, so don't
      // inspect these resources.
      continue;
    }

    // Now discover any resources that are the only resources served
    // on their hostname. These resources are considered violations.
    ResourceSet lone_dns_resources;
    PopulateLoneDnsResources(
        rule_input, host_resource_map, &lone_dns_resources);

    if (lone_dns_resources.size() > 0) {
      // Create a new result instance for the resources we discovered.
      AppendResult(lone_dns_resources,
                   host_resource_map.size() > lone_dns_resources.size(),
                   provider);
    }
  }

  return true;
}

void MinimizeDnsLookups::FormatResults(const ResultVector& results,
                                       RuleFormatter* formatter) {
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

  UrlBlockFormatter* body = formatter->AddUrlBlock(
      // TRANSLATOR: Heading that explains why the URLs that follow
      // should be moved to different hostnames. "DNS" should remain
      // in the translated format string.
      _("The hostnames of the following urls only serve one "
        "resource each. Avoid the extra DNS "
        "lookups by serving these resources from existing hostnames."));

  for (std::vector<std::string>::const_iterator iter = violation_urls.begin(),
           end = violation_urls.end();
       iter != end;
       ++iter) {
    body->AddUrl(*iter);
  }
}

int MinimizeDnsLookups::ComputeScore(const InputInformation& input_info,
                                     const RuleResults& results) {
  int num_violations = 0;
  for (int idx = 0, end = results.results_size(); idx < end; ++idx) {
    const Result& result = results.results(idx);
    num_violations += result.savings().dns_requests_saved();
  }
  const int num_hosts = input_info.number_hosts();
  if (num_hosts <= 0 || num_hosts < num_violations) {
    LOG(DFATAL) << "Bad num_hosts " << num_hosts
                << " compared to num_violations " << num_violations;
    return -1;
  }
  return 100 * (num_hosts - num_violations) / num_hosts;
}

}  // namespace rules

}  // namespace pagespeed
