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

#ifdef PAGESPEED_PNG_OPTIMIZER_GIF_READER
#include "pagespeed/image_compression/gif_reader.h"
#endif  // PAGESPEED_PNG_OPTIMIZER_GIF_READER

#include "pagespeed/image_compression/jpeg_optimizer.h"
#include "pagespeed/image_compression/png_optimizer.h"
#include "pagespeed/proto/pagespeed_output.pb.h"

namespace pagespeed {

namespace rules {

OptimizeImages::OptimizeImages(bool save_optimized_content)
    : save_optimized_content_(save_optimized_content) {
}

const char* OptimizeImages::name() const {
  return "OptimizeImages";
}

const char* OptimizeImages::header() const {
  return "Optimize images";
}

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

    std::string compressed;
    if (type == JPEG) {
      if (!image_compression::OptimizeJpeg(original, &compressed)) {
        error = true;
        continue;
      }
    } else if (type == PNG) {
      image_compression::PngReader reader;
      if (!image_compression::PngOptimizer::OptimizePng(reader,
                                                        original,
                                                        &compressed)) {
        error = true;
        continue;
      }
#ifdef PAGESPEED_PNG_OPTIMIZER_GIF_READER
    } else if (type == GIF) {
      image_compression::GifReader reader;
      if (!image_compression::PngOptimizer::OptimizePng(reader,
                                                        original,
                                                        &compressed)) {
        error = true;
        continue;
      }
#endif
    } else {
      continue;
    }

    const int bytes_saved = original_size - compressed.size();
    if (bytes_saved <= 0) {
      continue;
    }

    Result* result = results->add_results();
    result->set_rule_name(name());

    Savings* savings = result->mutable_savings();
    savings->set_response_bytes_saved(bytes_saved);

    result->add_resource_urls(resource.GetRequestUrl());

    if (save_optimized_content_) {
      result->set_optimized_content(compressed);
    }
  }

  return !error;
}

void OptimizeImages::FormatResults(const ResultVector& results,
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

  if (total_bytes_saved == 0) {
    return;
  }

  Argument arg(Argument::BYTES, total_bytes_saved);
  Formatter* body = formatter->AddChild("Optimizing the following image "
                                        "resources could reduce "
                                        "their size by $1.", arg);

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
    Argument savings(Argument::BYTES, result.savings().response_bytes_saved());

    std::string format_str = "Compressing $1 could save $2.";
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
