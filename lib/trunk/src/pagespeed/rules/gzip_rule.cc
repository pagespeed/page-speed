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

#include "pagespeed/rules/gzip_rule.h"

#include <string>

#include "base/string_util.h"
#include "pagespeed/core/formatter.h"
#include "pagespeed/core/pagespeed_input.h"
#include "pagespeed/core/pagespeed_output.pb.h"
#include "pagespeed/core/resource.h"
#include "pagespeed/rules/gzip_details.pb.h"

namespace pagespeed {

GzipRule::GzipRule() {
}

bool GzipRule::AppendResults(const PagespeedInput& input,
                             Results* results) {
  for (int idx = 0, num = input.num_resources(); idx < num; ++idx) {
    const Resource& resource = input.GetResource(idx);
    if (!isViolation(resource)) {
      continue;
    }

    Result* result = results->add_results();
    result->set_rule_name("GzipRule");

    int length = GetContentLength(resource);
    int bytes_saved = 2 * length / 3;

    ResultDetails* details = result->mutable_details();
    GzipDetails* gzip_details = details->MutableExtension(
        GzipDetails::message_set_extension);

    GzipDetails::PerUrlSavings* url_savings = gzip_details->add_url_savings();
    url_savings->set_url(resource.GetRequestUrl());
    url_savings->set_saved_bytes(bytes_saved);

    Savings* savings = result->mutable_savings();
    savings->set_response_bytes_saved(bytes_saved);
  }

  return true;
}

void GzipRule::FormatResults(const Results& results, Formatter* formatter) {
  Formatter* header = formatter->AddChild("Enable Gzip");

  int total_bytes_saved = 0;

  for (int result_idx = 0; result_idx < results.results_size(); result_idx++) {
    const Result& result = results.results(result_idx);
    const Savings& savings = result.savings();
    total_bytes_saved += savings.response_bytes_saved();
  }

  Argument arg(Argument::BYTES, total_bytes_saved);
  Formatter* body = header->AddChild("Compressing the following "
                                     "resources with gzip could reduce "
                                     "their transfer size by about two "
                                     "thirds (~$1).",
                                     arg);

  for (int result_idx = 0; result_idx < results.results_size(); result_idx++) {
    const Result& result = results.results(result_idx);
    const ResultDetails& details = result.details();
    const GzipDetails& gzip_details = details.GetExtension(
        GzipDetails::message_set_extension);

    for (int idx = 0; idx < gzip_details.url_savings_size(); idx++) {
      const GzipDetails::PerUrlSavings& url_savings =
          gzip_details.url_savings(idx);

      Argument url(Argument::URL, url_savings.url());
      Argument savings(Argument::BYTES, url_savings.saved_bytes());
      body->AddChild("Compressing $1 could save ~$2", url,
                     savings);
    }
  }
}

bool GzipRule::isCompressed(const Resource& resource) const {
  const std::string& encoding = resource.GetResponseHeader("Content-Encoding");
  return encoding == "gzip" || encoding == "deflate";
}

bool GzipRule::isText(const Resource& resource) const {
  ResourceType type = resource.GetResourceType();
  ResourceType text_types[] = { HTML, TEXT, JS, CSS };
  for (int idx = 0; idx < arraysize(text_types); ++idx) {
    if (type == text_types[idx]) {
      return true;
    }
  }

  return false;
}

bool GzipRule::isViolation(const Resource& resource) const {
  return !isCompressed(resource) &&
      isText(resource) &&
      GetContentLength(resource) >= 150;
}

int GzipRule::GetContentLength(const Resource& resource) const {
  const std::string& length_header =
      resource.GetResponseHeader("Content-Length");
  if (!length_header.empty()) {
    int output = 0;
    bool status = StringToInt(length_header, &output);
    DCHECK(status);
    return output;
  } else {
    return resource.GetResponseBody().size();
  }

  return 0;
}

}  // namespace pagespeed
