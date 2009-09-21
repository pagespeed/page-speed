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

#include "pagespeed/rules/minify_javascript.h"

#include <string>

#include "base/logging.h"
#include "pagespeed/core/formatter.h"
#include "pagespeed/core/pagespeed_input.h"
#include "pagespeed/core/resource.h"
#include "pagespeed/proto/pagespeed_output.pb.h"
#include "third_party/jsmin/cpp/jsmin.h"

namespace pagespeed {

namespace rules {

MinifyJavaScript::MinifyJavaScript() : Rule("MinifyJavaScript") {
}

bool MinifyJavaScript::AppendResults(const PagespeedInput& input,
                                     Results* results) {
  bool error = false;
  for (int idx = 0, num = input.num_resources(); idx < num; ++idx) {
    const Resource& resource = input.GetResource(idx);
    if (resource.GetResourceType() != JS) {
      continue;
    }

    jsmin::Minifier minifier(resource.GetResponseBody().c_str());
    std::string minified_js;
    if (!minifier.GetMinifiedOutput(&minified_js)) {
      error = true;
      continue;
    }

    const int bytes_saved =
        resource.GetResponseBody().size() - minified_js.size();
    if (bytes_saved <= 0) {
      continue;
    }

    Result* result = results->add_results();
    result->set_rule_name(name());

    Savings* savings = result->mutable_savings();
    savings->set_response_bytes_saved(bytes_saved);

    result->add_resource_urls(resource.GetRequestUrl());
  }

  return !error;
}

void MinifyJavaScript::FormatResults(const ResultVector& results,
                                     Formatter* formatter) {
  Formatter* header = formatter->AddChild("Minify JavaScript");

  int total_bytes_saved = 0;

  for (ResultVector::const_iterator iter = results.begin(),
           end = results.end();
       iter != end;
       ++iter) {
    const Result& result = **iter;
    const Savings& savings = result.savings();
    total_bytes_saved += savings.response_bytes_saved();
  }

  Argument arg(Argument::BYTES, total_bytes_saved);
  Formatter* body = header->AddChild("Minifying the following JavaScript "
                                     "resources using JSMin could reduce "
                                     "their size by $1.", arg);

  for (ResultVector::const_iterator iter = results.begin(),
           end = results.end();
       iter != end;
       ++iter) {
    const Result& result = **iter;
    CHECK(result.resource_urls_size() == 1);
    Argument url(Argument::URL, result.resource_urls(0));
    Argument savings(Argument::BYTES, result.savings().response_bytes_saved());
    body->AddChild("Minifying $1 could save $2", url, savings);
  }
}

}  // namespace rules

}  // namespace pagespeed
