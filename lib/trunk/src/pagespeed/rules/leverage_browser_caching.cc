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

#include "pagespeed/rules/leverage_browser_caching.h"

#include <algorithm>  // for stable_sort()

#include "base/logging.h"
#include "pagespeed/core/formatter.h"
#include "pagespeed/core/pagespeed_input.h"
#include "pagespeed/core/resource.h"
#include "pagespeed/core/resource_util.h"
#include "pagespeed/core/result_provider.h"
#include "pagespeed/core/rule_input.h"
#include "pagespeed/core/uri_util.h"
#include "pagespeed/l10n/l10n.h"
#include "pagespeed/proto/pagespeed_output.pb.h"

namespace {

const int64 kMillisInAnHour = 1000 * 60 * 60;
const int64 kMinAgeForThirdPartyContent = kMillisInAnHour * 12;
const int64 kMinAgeForSameDomainContent = kMillisInAnHour * 7 * 24;

// Extract the freshness lifetime from the result object.
int64 GetFreshnessLifetimeMillis(const pagespeed::Result &result) {
  const pagespeed::ResultDetails& details = result.details();
  if (!details.HasExtension(pagespeed::CachingDetails::message_set_extension)) {
    LOG(DFATAL) << "Missing required extension.";
    return -1;
  }

  const pagespeed::CachingDetails& caching_details = details.GetExtension(
      pagespeed::CachingDetails::message_set_extension);
  if (caching_details.is_heuristically_cacheable()) {
    if (caching_details.has_freshness_lifetime_millis()) {
      LOG(DFATAL) << "Details has a freshness_lifetime_millis "
                  << "and is_heuristically_cacheable.";
      return -1;
    }

    return 0;
  }

  return caching_details.freshness_lifetime_millis();
}

int64 ComputeAverageFreshnessLifetimeMillis(
    const pagespeed::InputInformation& input_info,
    const pagespeed::RuleResults& results) {
  if (results.results_size() <= 0) {
    LOG(DFATAL) << "Unexpected inputs: " << results.results_size();
    return -1;
  }
  const int number_static_resources = input_info.number_static_resources();

  // Any results that weren't flagged by this rule are properly
  // cached. This computation makes assumptions about the
  // implementation of AppendResults(). See the NOTE comment at the
  // top of that function for more details.
  const int number_properly_cached_resources =
      number_static_resources - results.results_size();
  if (number_properly_cached_resources < 0) {
    LOG(DFATAL) << "Number of results exceeds number of static resources.";
    return -1;
  }

  // Sum all of the freshness lifetimes of the results, so we can
  // compute an average.
  int64 freshness_lifetime_sum = 0;
  for (int i = 0, num = results.results_size(); i < num; ++i) {
    int64 resource_freshness_lifetime =
        GetFreshnessLifetimeMillis(results.results(i));
    if (resource_freshness_lifetime < 0) {
      // An error occurred.
      return -1;
    }
    freshness_lifetime_sum += resource_freshness_lifetime;
  }

  // In computing the score, we also need to account for the resources
  // that are properly cached, adding the target caching lifetime for
  // each such resource.
  freshness_lifetime_sum +=
      (number_properly_cached_resources * kMinAgeForSameDomainContent);

  return freshness_lifetime_sum / number_static_resources;
}

// StrictWeakOrdering that sorts by freshness lifetime
struct SortByFreshnessLifetime {
  bool operator()(const pagespeed::Result *a, const pagespeed::Result *b) {
    return GetFreshnessLifetimeMillis(*a) < GetFreshnessLifetimeMillis(*b);
    return true;
  }
};

// Resources served from third-party domains tend to have fixed URLs
// and thus it's not possible to include a fingerprint of the
// resource's contents in the URL. For these resources we expect a
// cache lifetime of 12 hours instead of one week. Note that we will
// fail to detect cases where a completely separate cookieless domain
// is being used (e.g. foo.com and foostatic.com, and will instead
// suggest caching for just 12 hours in those cases).
int64 GetExpectedFreshnessLifetimeForResource(
    const pagespeed::PagespeedInput& input,
    const pagespeed::Resource& resource) {
  if (input.primary_resource_url().empty()) {
    // If the primary resource URL wasn't specified, we can't be sure
    // whether the resource is on the same or a different domain. For
    // backward compatibility, we default to a freshness lifetime of a
    // week.
    return kMinAgeForSameDomainContent;
  }

  const std::string primary_resource_domain =
      pagespeed::uri_util::GetDomainAndRegistry(input.primary_resource_url());
  const std::string resource_domain =
      pagespeed::uri_util::GetDomainAndRegistry(resource.GetRequestUrl());
  if (primary_resource_domain == resource_domain) {
    return kMinAgeForSameDomainContent;
  } else {
    return kMinAgeForThirdPartyContent;
  }
}

}  // namespace

namespace pagespeed {

namespace rules {

LeverageBrowserCaching::LeverageBrowserCaching()
    : pagespeed::Rule(pagespeed::InputCapabilities()) {}

const char* LeverageBrowserCaching::name() const {
  return "LeverageBrowserCaching";
}

UserFacingString LeverageBrowserCaching::header() const {
  // TRANSLATOR: Rule name. A longer description would be "Specify
  // proper caching expirations for the resources on the web
  // page". Caching expirations are attached to each file on a web
  // page and instruct the browser to keep a copy of the resource
  // locally so it doesn't need to request that resource again. You
  // can read the documentation on the page speed web site for more
  // details.
  return _("Leverage browser caching");
}

bool LeverageBrowserCaching::AppendResults(const RuleInput& rule_input,
                                           ResultProvider* provider) {
  // NOTE: It's important that this rule only include results returned
  // from IsLikelyStaticResource. The logic in
  // ComputeAverageFreshnessLifetimeMillis assumes that the Results
  // emitted by this rule is the intersection of those that return
  // true for IsLikelyStaticResource and those that have an explicit
  // freshness lifetime less than kMinAgeForSameDomainContent (the
  // computation of number_properly_cached_resources makes this
  // assumption). If AppendResults changes such that this is no longer
  // true, the computation of number_properly_cached_resources will
  // need to change to match.
  const PagespeedInput& input = rule_input.pagespeed_input();
  for (int i = 0, num = input.num_resources(); i < num; ++i) {
    const Resource& resource = input.GetResource(i);
    if (!resource_util::IsLikelyStaticResource(resource)) {
      continue;
    }

    int64 freshness_lifetime_millis = 0;
    bool has_freshness_lifetime = resource_util::GetFreshnessLifetimeMillis(
        resource, &freshness_lifetime_millis);

    if (has_freshness_lifetime) {
      if (freshness_lifetime_millis <= 0) {
        // This should never happen.
        LOG(ERROR) << "Explicitly non-cacheable resources should "
                   << "not pass IsLikelyStaticResource test.";
        continue;
      }

      const int64 target_freshness_lifetime_millis =
          GetExpectedFreshnessLifetimeForResource(input, resource);

      if (freshness_lifetime_millis >= target_freshness_lifetime_millis) {
        continue;
      }
    }

    Result* result = provider->NewResult();
    ResultDetails* details = result->mutable_details();
    CachingDetails* caching_details = details->MutableExtension(
        CachingDetails::message_set_extension);
    // At this point, the resource either has an explicit freshness
    // lifetime, or it's heuristically cacheable. So we need to fill
    // out the appropriate field in the details structure.
    if (has_freshness_lifetime) {
      caching_details->set_freshness_lifetime_millis(freshness_lifetime_millis);
    } else {
      caching_details->set_is_heuristically_cacheable(true);
    }
    result->add_resource_urls(resource.GetRequestUrl());
  }
  return true;
}

void LeverageBrowserCaching::FormatResults(const ResultVector& results,
                                           RuleFormatter* formatter) {
  if (results.empty()) {
    return;
  }

  UrlBlockFormatter* body = formatter->AddUrlBlock(
      // TRANSLATOR: Heading that indicates which resources should
      // have a longer cache freshness lifetime. Here "freshness
      // lifetime" means the length of the period of time that the
      // file can be reused without checking to see if there is a
      // newer version of the file available.
      _("The following cacheable resources have a short "
        "freshness lifetime. Specify an expiration at least one "
        "week in the future for the following resources:"));

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
    if (!caching_details.has_freshness_lifetime_millis() &&
        !caching_details.is_heuristically_cacheable()) {
      // We expect the resource to either have an explicit
      // freshness_lifetime_millis or that it's heuristically
      // cacheable.
      LOG(DFATAL) << "Details structure is missing fields.";
    }

    if (caching_details.has_freshness_lifetime_millis()) {
      body->AddUrlResult(
          not_localized("$1 ($2)"), UrlArgument(result.resource_urls(0)),
          DurationArgument(caching_details.freshness_lifetime_millis()));
    } else {
      // TRANSLATOR: Item describing a single URL that violates the
      // LeverageBrowserCaching rule by not having a cache expiration.
      // "$1" is a format string that will be replaced by the URL.
      body->AddUrlResult(_("$1 (expiration not specified)"),
                         UrlArgument(result.resource_urls(0)));
    }
  }
}

int LeverageBrowserCaching::ComputeScore(const InputInformation& input_info,
                                         const RuleResults& results) {
  int64 avg_freshness_lifetime =
      ComputeAverageFreshnessLifetimeMillis(input_info, results);
  if (avg_freshness_lifetime < 0) {
    // An error occurred, so we cannot generate a score for this rule.
    return -1;
  }

  if (avg_freshness_lifetime > kMinAgeForSameDomainContent) {
    LOG(DFATAL) << "Average freshness lifetime " << avg_freshness_lifetime
                << " exceeds max suggested freshness lifetime "
                << kMinAgeForSameDomainContent;
    avg_freshness_lifetime = kMinAgeForSameDomainContent;
  }
  return static_cast<int>(
      100 * avg_freshness_lifetime / kMinAgeForSameDomainContent);
}

double LeverageBrowserCaching::ComputeResultImpact(
    const InputInformation& input_info, const Result& result) {
  const CachingDetails& caching_details = result.details().GetExtension(
      CachingDetails::message_set_extension);
  double lifetime =
      static_cast<double>(caching_details.freshness_lifetime_millis());
  if (lifetime < 0.0 || lifetime > kMinAgeForSameDomainContent) {
    LOG(DFATAL) << "Invalid freshness lifetime: " << lifetime;
    lifetime = 0.0;
  }
  const ClientCharacteristics& client = input_info.client_characteristics();
  // TODO(mdsteele): We should take into account not only the cost of the
  //   requests, but the cost of the bytes transferred over the net rather than
  //   taken from cache.
  return client.requests_weight() * client.expected_cache_hit_rate() *
      (1.0 - lifetime / static_cast<double>(kMinAgeForSameDomainContent));
}

}  // namespace rules

}  // namespace pagespeed
