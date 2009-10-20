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

#include "pagespeed/rules/optimize_images.h"

#include <string>

#include "base/logging.h"
#include "pagespeed/core/formatter.h"
#include "pagespeed/core/pagespeed_input.h"
#include "pagespeed/core/resource.h"
#include "pagespeed/image_compression/jpeg_optimizer.h"
#include "pagespeed/image_compression/png_optimizer.h"
#include "pagespeed/proto/pagespeed_output.pb.h"

namespace pagespeed {

namespace rules {

OptimizeImages::OptimizeImages() : Rule("OptimizeImages") {}

bool OptimizeImages::AppendResults(const PagespeedInput& input,
                                   Results* results) {
  bool error = false;
  for (int idx = 0, num = input.num_resources(); idx < num; ++idx) {
    const Resource& resource = input.GetResource(idx);
    if (resource.GetResourceType() != IMAGE) {
      continue;
    }

    const ImageType type = resource.GetImageType();
    const std::string& original = resource.GetResponseBody();
    const int original_size = original.size();
    int compressed_size = original_size;

    if (type == JPEG) {
      std::string compressed;
      if (!image_compression::JpegOptimizer::OptimizeJpeg(original,
                                                          &compressed)) {
        error = true;
        continue;
      }
      compressed_size = compressed.size();
    } else if (type == PNG) {
      std::string compressed;
      if (!image_compression::PngOptimizer::OptimizePng(original,
                                                        &compressed)) {
        error = true;
        continue;
      }
      compressed_size = compressed.size();
    } else {
      // TODO(mdsteele) add a case for GIF (and maybe other formats?)
      continue;
    }

    const int bytes_saved = original_size - compressed_size;
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

void OptimizeImages::FormatResults(const ResultVector& results,
                                   Formatter* formatter) {
  Formatter* header = formatter->AddChild("Optimize Images");

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
  Formatter* body = header->AddChild("Optimizing the following image "
                                     "resources could reduce "
                                     "their size by $1.", arg);

  for (ResultVector::const_iterator iter = results.begin(),
           end = results.end();
       iter != end;
       ++iter) {
    const Result& result = **iter;
    CHECK(result.resource_urls_size() == 1);
    Argument url(Argument::URL, result.resource_urls(0));
    Argument savings(Argument::BYTES, result.savings().response_bytes_saved());
    body->AddChild("Compressing $1 could save $2", url, savings);
  }
}

}  // namespace rules

}  // namespace pagespeed
