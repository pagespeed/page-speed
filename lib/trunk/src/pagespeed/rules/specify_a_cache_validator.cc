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

#include "pagespeed/rules/specify_a_cache_validator.h"

#include "base/logging.h"
#include "pagespeed/core/formatter.h"
#include "pagespeed/core/pagespeed_input.h"
#include "pagespeed/core/resource.h"
#include "pagespeed/core/resource_util.h"
#include "pagespeed/core/result_provider.h"
#include "pagespeed/core/rule_input.h"
#include "pagespeed/l10n/l10n.h"
#include "pagespeed/proto/pagespeed_output.pb.h"

namespace {

bool HasValidLastModifiedHeader(const pagespeed::Resource& resource) {
  const std::string& last_modified =
      resource.GetResponseHeader("Last-Modified");
  if (last_modified.empty()) {
    return false;
  }
  int64 last_modified_value = 0;
  if (!pagespeed::resource_util::ParseTimeValuedHeader(
          last_modified.c_str(), &last_modified_value)) {
    return false;
  }
  return true;
}

bool HasETagHeader(const pagespeed::Resource& resource) {
  const std::string& etag = resource.GetResponseHeader("ETag");
  return !etag.empty();
}

}  // namespace

namespace pagespeed {

namespace rules {

SpecifyACacheValidator::SpecifyACacheValidator()
    : pagespeed::Rule(pagespeed::InputCapabilities()) {
}

const char* SpecifyACacheValidator::name() const {
  return "SpecifyACacheValidator";
}

UserFacingString SpecifyACacheValidator::header() const {
  // TRANSLATOR: The name of a Page Speed rule that tells users to ensure that
  // their server provides a "cache validator" for each served resource -- that
  // is, an HTTP header that indicates to the browser how to check whether a
  // particular item in its cache is still valid.  This is displayed in a list
  // of rule names that Page Speed generates.
  return _("Specify a cache validator");
}

bool SpecifyACacheValidator::AppendResults(const RuleInput& rule_input,
                                           ResultProvider* provider) {
  const PagespeedInput& input = rule_input.pagespeed_input();
  for (int i = 0, num = input.num_resources(); i < num; ++i) {
    const Resource& resource = input.GetResource(i);
    if (!resource_util::IsLikelyStaticResource(resource)) {
      // Probably not a static resource, so don't suggest using a
      // cache validator.
      continue;
    }

    if (HasValidLastModifiedHeader(resource) ||
        HasETagHeader(resource)) {
      // The response already has a valid cache validator.
      continue;
    }

    // No savings data is needed for this resource. All cache
    // validators have the same cost/benefit.
    Result* result = provider->NewResult();
    result->add_resource_urls(resource.GetRequestUrl());
  }
  return true;
}

void SpecifyACacheValidator::FormatResults(const ResultVector& results,
                                           RuleFormatter* formatter) {
  if (results.empty()) {
    return;
  }

  UrlBlockFormatter* body = formatter->AddUrlBlock(
      // TRANSLATOR: Header at the top of a list of URLs that Page Speed
      // detected as lacking a "cache validator" -- that is, an HTTP header
      // that indicates to the browser how to check whether a particular item
      // in its cache is still valid.  It describes the problem to the user,
      // and tells them how to fix it by configuring their server to include a
      // "Last-Modified" HTTP header, or an "ETag" HTTP header, either of which
      // can serve as a cache validator.  Note that "Last-Modified" and "ETag"
      // are code and should not be translated.
      _("The following resources are missing a cache validator. Resources "
        "that do not specify a cache validator cannot be refreshed "
        "efficiently. Specify a Last-Modified or ETag header to enable cache "
        "validation for the following resources:"));

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
    body->AddUrl(result.resource_urls(0));
  }
}

int SpecifyACacheValidator::ComputeScore(const InputInformation& input_info,
                                         const RuleResults& results) {
  // Every static/cacheable resource should have a cache validator. So
  // we compute the score as the number of static resources with a
  // validator over the total number of static resources.
  const int num_static_resources = input_info.number_static_resources();
  if (num_static_resources == 0) {
    return 100;
  }
  const int num_non_violations = num_static_resources - results.results_size();
  DCHECK(num_non_violations >= 0);
  return 100 * num_non_violations / num_static_resources;
}

double SpecifyACacheValidator::ComputeResultImpact(
    const InputInformation& input_info, const Result& result) {
  const ClientCharacteristics& client = input_info.client_characteristics();
  // TODO(mdsteele): This 0.5 here is a gross hack, meant to express that this
  //   rule is not as important as LeverageBrowserCaching.  We should replace
  //   this formula with something less arbitrary.
  return 0.5 * client.requests_weight() * client.expected_cache_hit_rate();
}

}  // namespace rules

}  // namespace pagespeed
