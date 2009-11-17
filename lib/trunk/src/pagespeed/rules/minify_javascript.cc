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

namespace {

struct MinifierOutput {
 public:
  MinifierOutput(int bytes_saved) : bytes_saved_(bytes_saved) {
  }

  MinifierOutput(int bytes_saved, const std::string& minified_js)
      : bytes_saved_(bytes_saved),
        minified_js_(new std::string(minified_js)) {
  }

  int bytes_saved() { return bytes_saved_; }
  std::string* minified_js() { return minified_js_.get(); }
 private:
  int bytes_saved_;
  scoped_ptr<std::string> minified_js_;

  DISALLOW_COPY_AND_ASSIGN(MinifierOutput);
};

MinifierOutput* MinifyJs(const std::string& input, bool save_optimized_output) {
  if (save_optimized_output) {
    std::string minified_js;
    if (!jsmin::MinifyJs(input, &minified_js)) {
      return NULL;  // error
    }

    const int bytes_saved = input.size() - minified_js.size();
    if (bytes_saved > 0) {
      return new MinifierOutput(bytes_saved, minified_js);
    } else {
      return new MinifierOutput(0);
    }
  } else {
    int minified_js_size = 0;
    if (!jsmin::GetMinifiedJsSize(input, &minified_js_size)) {
      return NULL;  // error
    }

    const int bytes_saved = input.size() - minified_js_size;
    return new MinifierOutput(bytes_saved);
  }
}

}  // namespace

namespace pagespeed {

namespace rules {

MinifyJavaScript::MinifyJavaScript() {
}

const char* MinifyJavaScript::name() const {
  return "MinifyJavaScript";
}

const char* MinifyJavaScript::header() const {
  return "Minify JavaScript";
}

bool MinifyJavaScript::AppendResults(const PagespeedInput& input,
                                     Results* results) {
  bool error = false;
  for (int idx = 0, num = input.num_resources(); idx < num; ++idx) {
    const Resource& resource = input.GetResource(idx);
    if (resource.GetResourceType() != JS) {
      continue;
    }

    bool save_optimized_content = true;
    scoped_ptr<MinifierOutput> minified_output(
        MinifyJs(resource.GetResponseBody(), save_optimized_content));

    if (!minified_output.get()) {
      error = true;
      continue;
    }

    const int bytes_saved = minified_output->bytes_saved();
    if (bytes_saved <= 0) {
      continue;
    }

    Result* result = results->add_results();
    result->set_rule_name(name());

    Savings* savings = result->mutable_savings();
    savings->set_response_bytes_saved(bytes_saved);

    result->add_resource_urls(resource.GetRequestUrl());

    const std::string* minified_js = minified_output->minified_js();
    if (minified_js) {
      result->set_optimized_content(*minified_js);
    }
  }

  return !error;
}

void MinifyJavaScript::FormatResults(const ResultVector& results,
                                     Formatter* formatter) {
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
  Formatter* body = formatter->AddChild("Minifying the following JavaScript "
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

    std::string format_str = "Minifying $1 could save $2.";
    std::vector<const Argument*> args;
    args.push_back(&url);
    args.push_back(&savings);

    FormatterParameters formatter_args(&format_str, &args);

    if (result.has_optimized_content()) {
      formatter_args.set_optimized_content(&result.optimized_content());
    }

    body->AddChild(formatter_args);
  }
}

}  // namespace rules

}  // namespace pagespeed
