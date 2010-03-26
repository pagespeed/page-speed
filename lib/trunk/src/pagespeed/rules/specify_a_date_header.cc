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

#include "pagespeed/rules/specify_a_date_header.h"

#include "base/logging.h"
#include "pagespeed/core/formatter.h"
#include "pagespeed/core/pagespeed_input.h"
#include "pagespeed/core/resource.h"
#include "pagespeed/core/resource_util.h"
#include "pagespeed/proto/pagespeed_output.pb.h"

namespace {

bool ShouldHaveADateHeader(const pagespeed::Resource& resource) {
  // Based on
  // http://www.w3.org/Protocols/rfc2616/rfc2616-sec14.html#sec14.18
  const int code = resource.GetResponseStatusCode();
  if (500 <= code && code < 600) {
    // 5xx server error responses are not required to include a Date
    // header.
    return false;
  }

  switch (resource.GetResponseStatusCode()) {
    case 100:
    case 101:
      return false;
    default:
      // All other responses should include a Date header.
      return true;
  }
}

}  // namespace

namespace pagespeed {

namespace rules {

SpecifyADateHeader::SpecifyADateHeader() {
}

const char* SpecifyADateHeader::name() const {
  return "SpecifyADateHeader";
}

const char* SpecifyADateHeader::header() const {
  return "Specify a date header";
}

const char* SpecifyADateHeader::documentation_url() const {
  return "caching.html#LeverageBrowserCaching";
}

bool SpecifyADateHeader::AppendResults(const PagespeedInput& input,
                                       Results* results) {
  for (int i = 0, num = input.num_resources(); i < num; ++i) {
    const Resource& resource = input.GetResource(i);
    if (!ShouldHaveADateHeader(resource)) {
      continue;
    }

    if (resource_util::HasExplicitNoCacheDirective(resource)) {
      // The Date header is used to validate the freshness lifetime of
      // a resource. But if the resource is already marked as
      // explicitly non-cacheable, the Date header is unimportant, so
      // skip it.
      continue;
    }

    const std::string& date = resource.GetResponseHeader("Date");
    int64 date_value_millis = 0;
    if (resource_util::ParseTimeValuedHeader(
            date.c_str(), &date_value_millis)) {
      // The resource has a valid date header, so exclude it from the
      // result set.
      continue;
    }

    Result* result = results->add_results();
    result->set_rule_name(name());

    // TODO: populate savings.

    result->add_resource_urls(resource.GetRequestUrl());
  }
  return true;
}

void SpecifyADateHeader::FormatResults(const ResultVector& results,
                                       Formatter* formatter) {
  if (results.empty()) {
    return;
  }

  Formatter* body = formatter->AddChild(
      "The following resources are missing a valid date header. Resources "
      "that do not specify a valid date header may not be cached by some "
      "browsers or proxies:");

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
