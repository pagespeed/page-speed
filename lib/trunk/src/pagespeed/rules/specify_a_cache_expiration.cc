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
                                            Results* results) {
  for (int i = 0, num = input.num_resources(); i < num; ++i) {
    const Resource& resource = input.GetResource(i);
    if (resource_util::HasExplicitFreshnessLifetime(resource)) {
      // The resource has a cache expiration, so exclude it from the
      // result set.
      continue;
    }

    if (!resource_util::IsCacheableResource(resource)) {
      // The resource isn't cacheable, so don't include it in the
      // analysis.
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

    Result* result = results->add_results();
    result->set_rule_name(name());

    // TODO: populate savings.

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
      "Specify an expiration at least one month in the future for resources "
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

}  // namespace rules

}  // namespace pagespeed
