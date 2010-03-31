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

#include "pagespeed/rules/cache_static_resources_aggressively.h"

#include <algorithm>  // for stable_sort()

#include "base/logging.h"
#include "pagespeed/core/formatter.h"
#include "pagespeed/core/pagespeed_input.h"
#include "pagespeed/core/resource.h"
#include "pagespeed/core/resource_util.h"
#include "pagespeed/core/result_provider.h"
#include "pagespeed/proto/pagespeed_output.pb.h"

namespace {

const int64 kMillisInADay = 1000 * 60 * 60 * 24;
const int64 kMillisInAWeek = kMillisInADay * 7;

// Extract the freshness lifetime from the result object.
int64 GetFreshnessLifetimeMillis(const pagespeed::Result &result) {
  const pagespeed::ResultDetails& details = result.details();
  if (!details.HasExtension(pagespeed::CachingDetails::message_set_extension)) {
    LOG(DFATAL) << "Missing required extension.";
    return 0;
  }

  const pagespeed::CachingDetails& caching_details = details.GetExtension(
      pagespeed::CachingDetails::message_set_extension);
  return caching_details.freshness_lifetime_millis();
}

int64 ComputeAverageFreshnessLifetimeMillis(
    const pagespeed::InputInformation& input_info,
    const pagespeed::ResultVector& results) {
  const int number_cacheable_static_resources =
      input_info.number_explicitly_cacheable_static_resources();
  if (number_cacheable_static_resources <= 0 || results.size() <= 0) {
    LOG(DFATAL) << "Unexpected inputs: " << number_cacheable_static_resources
                << ", " << results.size();
    return -1;
  }

  const int num_properly_cached_resources =
      number_cacheable_static_resources - results.size();
  if (num_properly_cached_resources < 0) {
    LOG(DFATAL) << "Number of results exceeds number of static resources.";
    return -1;
  }

  // Sum all of the freshness lifetimes of the results, so we can
  // compute an average.
  int64 freshness_lifetime_sum = 0;
  for (int i = 0, num = results.size(); i < num; ++i) {
    freshness_lifetime_sum += GetFreshnessLifetimeMillis(*results[i]);
  }

  // In computing the score, we also need to account for the resources
  // that are properly cached, adding a full caching lifetime for each
  // such resource.
  freshness_lifetime_sum += (num_properly_cached_resources * kMillisInAWeek);

  return freshness_lifetime_sum / number_cacheable_static_resources;
}

// StrictWeakOrdering that sorts by freshness lifetime
struct SortByFreshnessLifetime {
  bool operator()(const pagespeed::Result *a, const pagespeed::Result *b) {
    return GetFreshnessLifetimeMillis(*a) < GetFreshnessLifetimeMillis(*b);
    return true;
  }
};

}  // namespace

namespace pagespeed {

namespace rules {

CacheStaticResourcesAggressively::CacheStaticResourcesAggressively() {
}

const char* CacheStaticResourcesAggressively::name() const {
  return "CacheStaticResourcesAggressively";
}

const char* CacheStaticResourcesAggressively::header() const {
  return "Cache static resources aggressively";
}

const char* CacheStaticResourcesAggressively::documentation_url() const {
  return "caching.html#LeverageBrowserCaching";
}

bool CacheStaticResourcesAggressively::AppendResults(
    const PagespeedInput& input, ResultProvider* provider) {
  for (int i = 0, num = input.num_resources(); i < num; ++i) {
    const Resource& resource = input.GetResource(i);
    if (!resource_util::IsLikelyStaticResource(resource)) {
      continue;
    }

    int64 freshness_lifetime_millis = 0;
    if (!resource_util::GetFreshnessLifetimeMillis(
            resource, &freshness_lifetime_millis)) {
      continue;
    }

    if (freshness_lifetime_millis <= 0) {
      // This should never happen.
      LOG(ERROR) << "Explicitly non-cacheable resources should "
                 << "not pass IsLikelyStaticResource test.";
      continue;
    }

    if (freshness_lifetime_millis >= kMillisInAWeek) {
      continue;
    }

    Result* result = provider->NewResult();
    ResultDetails* details = result->mutable_details();
    CachingDetails* caching_details = details->MutableExtension(
        CachingDetails::message_set_extension);
    caching_details->set_freshness_lifetime_millis(freshness_lifetime_millis);
    result->add_resource_urls(resource.GetRequestUrl());
  }
  return true;
}

void CacheStaticResourcesAggressively::FormatResults(
    const ResultVector& results, Formatter* formatter) {
  if (results.empty()) {
    return;
  }

  Formatter* body = formatter->AddChild(
      "The following cacheable resources have a short "
      "freshness lifetime. Specify an expiration at least one "
      "week in the future for the following resources:");

  // Show the resources with the shortest freshness lifetime first.
  ResultVector sorted_results = results;
  std::stable_sort(sorted_results.begin(),
                   sorted_results.end(),
                   SortByFreshnessLifetime());

  for (ResultVector::const_iterator iter = sorted_results.begin(),
           end = sorted_results.end();
       iter != end;
       ++iter) {
    const Result& result = **iter;
    if (result.resource_urls_size() != 1) {
      LOG(DFATAL) << "Unexpected number of resource URLs.  Expected 1, Got "
                  << result.resource_urls_size() << ".";
      continue;
    }
    const ResultDetails& details = result.details();
    if (!details.HasExtension(CachingDetails::message_set_extension)) {
      LOG(DFATAL) << "Missing required extension.";
      continue;
    }

    const CachingDetails& caching_details = details.GetExtension(
        CachingDetails::message_set_extension);

    Argument url(Argument::URL, result.resource_urls(0));
    Argument freshness_lifetime(
        Argument::DURATION,
        caching_details.freshness_lifetime_millis());
    body->AddChild("$1 ($2)", url, freshness_lifetime);
  }
}

int CacheStaticResourcesAggressively::ComputeScore(
    const InputInformation& input_info, const ResultVector& results) {
  int64 avg_freshness_lifetime =
      ComputeAverageFreshnessLifetimeMillis(input_info, results);
  if (avg_freshness_lifetime > kMillisInAWeek) {
    LOG(DFATAL) << "Average freshness lifetime " << avg_freshness_lifetime
                << " exceeds max suggested freshness lifetime "
                << kMillisInAWeek;
    avg_freshness_lifetime = kMillisInAWeek;
  }
  return 100 * avg_freshness_lifetime / kMillisInAWeek;
}

}  // namespace rules

}  // namespace pagespeed
