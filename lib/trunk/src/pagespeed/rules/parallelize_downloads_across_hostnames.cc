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

#include "pagespeed/rules/parallelize_downloads_across_hostnames.h"

#include <algorithm>  // for partial_sort_copy
#include <string>
#include <vector>

#include "base/logging.h"
#include "pagespeed/core/formatter.h"
#include "pagespeed/core/pagespeed_input.h"
#include "pagespeed/core/resource.h"
#include "pagespeed/core/resource_util.h"
#include "pagespeed/core/result_provider.h"
#include "pagespeed/core/rule_input.h"
#include "pagespeed/l10n/l10n.h"
#include "pagespeed/proto/pagespeed_output.pb.h"

namespace pagespeed {

namespace rules {

namespace {

// TODO: We need to investigate what are good values for the below constants.

// Examine only the top N hostnames serving static resources:
const int kOptimalNumberOfHostnames = 2;
// If no one host serves more than N resources, report nothing:
const int kMinResourceThreshold = 25;
// Don't penalize the site until their busiest host is 50% busier than the
// average of the top kOptimalNumberOfHostnames hosts.
const double kMinBalanceThreshold = 0.5;

class SortByNumberOfResources {
 public:
  explicit SortByNumberOfResources(HostResourceMap* host_resource_map)
      : host_resource_map_(host_resource_map) {}

  bool operator()(const std::string& host1, const std::string& host2) {
    return ((*host_resource_map_)[host1].size() >
            (*host_resource_map_)[host2].size());
  }

 private:
  HostResourceMap* const host_resource_map_;
};

}  // namespace

ParallelizeDownloadsAcrossHostnames::ParallelizeDownloadsAcrossHostnames()
    : pagespeed::Rule(pagespeed::InputCapabilities(
        pagespeed::InputCapabilities::ONLOAD |
        pagespeed::InputCapabilities::REQUEST_START_TIMES)) {}

const char* ParallelizeDownloadsAcrossHostnames::name() const {
  return "ParallelizeDownloadsAcrossHostnames";
}

UserFacingString ParallelizeDownloadsAcrossHostnames::header() const {
  // TRANSLATOR: The name of a Page Speed rule that tells users to parallelize
  // downloads resources (image, css, javascript) across hostnames. This is
  // displayed in a list of rule names that Page Speed generates.
  return _("Parallelize downloads across hostnames");
}

bool ParallelizeDownloadsAcrossHostnames::
AppendResults(const RuleInput& rule_input, ResultProvider* provider) {
  const PagespeedInput& input = rule_input.pagespeed_input();
  const HostResourceMap& host_resource_map = *input.GetHostResourceMap();
  std::vector<std::string> hosts;
  HostResourceMap static_resource_hosts;

  // Collect all hosts and static resources.
  for (HostResourceMap::const_iterator iter1 = host_resource_map.begin(),
           end1 = host_resource_map.end(); iter1 != end1; ++iter1) {
    const std::string& host = iter1->first;
    hosts.push_back(host);
    const ResourceSet& resources = iter1->second;
    for (ResourceSet::const_iterator iter2 = resources.begin(),
             end2 = resources.end(); iter2 != end2; ++iter2) {
      const Resource* resource = *iter2;
      if (!input.IsResourceLoadedAfterOnload(*resource) &&
          resource_util::IsLikelyStaticResource(*resource)) {
        static_resource_hosts[host].insert(resource);
      }
    }
  }

  // If there are no hosts serving static resources, then we can quit now.
  if (static_resource_hosts.empty()) {
    return true;
  }

  // Sort hosts by the number of static resources served.
  std::vector<std::string> top_hosts(kOptimalNumberOfHostnames);
  SortByNumberOfResources sort_function(&static_resource_hosts);
  std::partial_sort_copy(hosts.begin(), hosts.end(),
                         top_hosts.begin(), top_hosts.end(),
                         sort_function);

  // If the top host has at most kMinResourceThreshold static resources, then
  // parallelization is probably overkill.
  const std::string& busiest_host = *top_hosts.begin();
  const ResourceSet& resources_on_busiest_host =
      static_resource_hosts[busiest_host];
  const int num_resources_on_busiest_host = resources_on_busiest_host.size();
  const int num_resources_above_threshold =
      num_resources_on_busiest_host - kMinResourceThreshold;
  if (num_resources_above_threshold <= 0) {
    return true;
  }

  // Calculate the average number of static resources across the top
  // kOptimalNumberOfHostnames hosts; we'll use this for scoring.
  int num_static_resources = 0;
  for (std::vector<std::string>::const_iterator iter = top_hosts.begin(),
           end = top_hosts.end(); iter != end; ++iter) {
    num_static_resources += static_resource_hosts[*iter].size();
  }
  const double average_num_static_resources =
      static_cast<double>(num_static_resources) /
      static_cast<double>(kOptimalNumberOfHostnames);
  if (average_num_static_resources <= 0.0) {
    LOG(DFATAL) << "average_num_static_resources <= 0.0";
    return false;
  }

  // Don't penalize the site unless it's sufficiently unbalanced.
  const double num_resources_above_average =
      static_cast<double>(num_resources_on_busiest_host) -
      average_num_static_resources;
  const double percentage_above_average =
      num_resources_above_average / average_num_static_resources;
  if (percentage_above_average < kMinBalanceThreshold) {
    return true;
  }

  Result* result = provider->NewResult();
  for (ResourceSet::const_iterator iter = resources_on_busiest_host.begin(),
           end = resources_on_busiest_host.end(); iter != end; ++iter) {
    const Resource* resource = *iter;
    result->add_resource_urls(resource->GetRequestUrl());
  }

  Savings* savings = result->mutable_savings();
  savings->set_critical_path_length_saved(
      static_cast<int>(num_resources_above_average));

  ResultDetails* details = result->mutable_details();
  ParallelizableHostDetails* host_details = details->MutableExtension(
      ParallelizableHostDetails::message_set_extension);
  host_details->set_host(busiest_host);

  return true;
}

void ParallelizeDownloadsAcrossHostnames::
FormatResults(const ResultVector& results, RuleFormatter* formatter) {
  for (ResultVector::const_iterator iter = results.begin(),
           end = results.end(); iter != end; ++iter) {
    const Result& result = **iter;
    const ResultDetails& details = result.details();
    if (details.HasExtension(
            ParallelizableHostDetails::message_set_extension)) {
      const ParallelizableHostDetails& host_details = details.GetExtension(
          ParallelizableHostDetails::message_set_extension);

      const int num_resources = result.resource_urls_size();
      UrlBlockFormatter* body = formatter->AddUrlBlock(
          // TRANSLATOR: Header at the top of a list of URLs that Page Speed
          // detected as from one host. It describes the problem to the user,
          // and tells them how to fix it by distributing the requests across
          // multiple hostnames. The "$1" will be replace by the number of the
          // URLs; the "$2" wil be replaced by the name of the host (e.g.
          // google.com).
          _("This page makes $1 parallelizable requests to $2.  Increase "
            "download parallelization by distributing these requests across "
            "multiple hostnames:"), IntArgument(num_resources),
          StringArgument(host_details.host()));

      for (int index = 0; index < num_resources; ++index) {
        body->AddUrl(result.resource_urls(index));
      }
    }
  }
}

}  // namespace rules

}  // namespace pagespeed
