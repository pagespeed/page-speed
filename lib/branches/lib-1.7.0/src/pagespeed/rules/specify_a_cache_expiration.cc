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

#include "pagespeed/rules/specify_a_cache_expiration.h"

#include "base/logging.h"
#include "pagespeed/core/formatter.h"
#include "pagespeed/core/pagespeed_input.h"
#include "pagespeed/core/resource.h"
#include "pagespeed/core/resource_util.h"
#include "pagespeed/core/result_provider.h"
#include "pagespeed/proto/pagespeed_output.pb.h"

namespace pagespeed {

namespace rules {

SpecifyACacheExpiration::SpecifyACacheExpiration() {
}

const char* SpecifyACacheExpiration::name() const {
  return "SpecifyACacheExpiration";
}

const char* SpecifyACacheExpiration::header() const {
  return "Specify a cache expiration";
}

const char* SpecifyACacheExpiration::documentation_url() const {
  return "caching.html#LeverageBrowserCaching";
}

bool SpecifyACacheExpiration::AppendResults(const PagespeedInput& input,
                                            ResultProvider* provider) {
  for (int i = 0, num = input.num_resources(); i < num; ++i) {
    const Resource& resource = input.GetResource(i);
    if (resource_util::HasExplicitFreshnessLifetime(resource)) {
      // The resource has a cache expiration, so exclude it from the
      // result set.
      continue;
    }

    if (!resource_util::IsCacheableResourceStatusCode(
            resource.GetResponseStatusCode())) {
      // The resource has a status code that isn't generally known to
      // be associated with cacheable resources, so exclude
      // it from the result set.
      continue;
    }

    const std::string& date = resource.GetResponseHeader("Date");
    int64 date_value_millis = 0;
    if (!resource_util::ParseTimeValuedHeader(
            date.c_str(), &date_value_millis)) {
      // The resource does not have a valid date header, so it might
      // not be possible to compute its freshness lifetime. Thus, we
      // should not warn about it here. The SpecifyADateHeader rule
      // will warn about this resource.
      continue;
    }

    Result* result = provider->NewResult();
    ResultDetails* details = result->mutable_details();
    CachingDetails* caching_details = details->MutableExtension(
        CachingDetails::message_set_extension);
    caching_details->set_is_likely_cacheable(
        resource_util::IsLikelyStaticResource(resource));
    result->add_resource_urls(resource.GetRequestUrl());
  }
  return true;
}

void SpecifyACacheExpiration::FormatResults(const ResultVector& results,
                                            Formatter* formatter) {
  if (results.empty()) {
    return;
  }

  Formatter* body = formatter->AddChild(
      "The following resources are missing a cache expiration. Resources "
      "that do not specify an expiration may not be cached by browsers. "
      "Specify an expiration at least one week in the future for resources "
      "that should be cached, and an expiration in the past for resources "
      "that should not be cached:");

  for (ResultVector::const_iterator iter = results.begin(),
           end = results.end();
       iter != end;
       ++iter) {
    const Result& result = **iter;
    if (result.resource_urls_size() != 1) {
      LOG(DFATAL) << "Unexpected number of resource URLs.  Expected 1, Got "
                  << result.resource_urls_size() << ".";
      continue;
    }
    Argument url(Argument::URL, result.resource_urls(0));
    body->AddChild("$1", url);
  }
}

int SpecifyACacheExpiration::ComputeScore(const InputInformation& input_info,
                                          const ResultVector& results) {
  // Almost every resource should have an expiration. A handful of
  // resources, such as 204 responses, are not cacheable by default,
  // and thus don't need a cache expiration. So technically the number
  // of candidate resources might be slightly less than the total
  // number of resources. However, for most sites the number of
  // 204-like responses is small, so including them in the candidate
  // set doesn't have much impact.
  const int num_candidate_resources = input_info.number_resources();
  const int num_non_violations = num_candidate_resources - results.size();
  return 100 * num_non_violations / num_candidate_resources;
}

}  // namespace rules

}  // namespace pagespeed
