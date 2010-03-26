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

SpecifyACacheValidator::SpecifyACacheValidator() {
}

const char* SpecifyACacheValidator::name() const {
  return "SpecifyACacheValidator";
}

const char* SpecifyACacheValidator::header() const {
  return "Specify a cache validator";
}

const char* SpecifyACacheValidator::documentation_url() const {
  return "caching.html#LeverageBrowserCaching";
}

bool SpecifyACacheValidator::AppendResults(const PagespeedInput& input,
                                           Results* results) {
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

    Result* result = results->add_results();
    result->set_rule_name(name());

    // TODO: populate savings.

    result->add_resource_urls(resource.GetRequestUrl());
  }
  return true;
}

void SpecifyACacheValidator::FormatResults(const ResultVector& results,
                                           Formatter* formatter) {
  if (results.empty()) {
    return;
  }

  Formatter* body = formatter->AddChild(
      "The following resources are missing a cache validator. Resources "
      "that do not specify a cache validator cannot be refreshed efficiently. "
      "Specify a Last-Modified or ETag header to enable cache validation "
      "for the following resources:");

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
