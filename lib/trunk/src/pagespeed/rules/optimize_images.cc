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

#include "pagespeed/core/resource.h"
#include "pagespeed/core/rule_input.h"

#include "pagespeed/image_compression/gif_reader.h"
#include "pagespeed/image_compression/jpeg_optimizer.h"
#include "pagespeed/image_compression/png_optimizer.h"
#include "pagespeed/l10n/l10n.h"
#include "pagespeed/proto/pagespeed_output.pb.h"

namespace pagespeed {

namespace rules {

namespace {

// This cost weight yields an avg score of 85 and a median score of 95
// for the top 100 websites.
const double kCostWeight = 3;

class ImageMinifier : public Minifier {
 public:
  explicit ImageMinifier(bool save_optimized_content)
      : save_optimized_content_(save_optimized_content) {}

  // Minifier interface:
  virtual const char* name() const;
  virtual UserFacingString header_format() const;
  virtual UserFacingString body_format() const;
  virtual UserFacingString child_format() const;
  virtual UserFacingString child_format_post_gzip() const;
  virtual const MinifierOutput* Minify(const Resource& resource,
                                       const RuleInput& input) const;

 private:
  bool save_optimized_content_;

  DISALLOW_COPY_AND_ASSIGN(ImageMinifier);
};

const char* ImageMinifier::name() const {
  return "OptimizeImages";
}

UserFacingString ImageMinifier::header_format() const {
  // TRANSLATOR: The name of a Page Speed rule that tells users to optimize the
  // images (better compression). This is displayed in a list of rule names that
  // Page Speed generates.
  return _("Optimize images");
}

UserFacingString ImageMinifier::body_format() const {
  // TRANSLATOR: Header at the top a list of image URLs that Page Speed
  // detected as can be optimized by using better compression. It describes the
  // problem to the user that the size of the images can be reduced. The
  // "SIZE_IN_BYTES" placeholder will be replaced by the absolute number of
  // bytes or kilobytes that can be saved (e.g. "5 bytes" or "23.2KiB"). The
  // "PERCENTAGE" placeholder will be replaced by the percent savings
  // (e.g. "50%").
  return _("Optimizing the following images could reduce their size "
           "by %(SIZE_IN_BYTES)s (%(PERCENTAGE)s reduction).");
}

UserFacingString ImageMinifier::child_format() const {
  // TRANSLATOR: Subheading that describes the savings possible from optimizing
  // a particular image resource via lossless compression.  The "SIZE_IN_BYTES"
  // placeholder will be replaced by the absolute number of bytes or kilobytes
  // that can be saved (e.g. "5 bytes" or "23.2KiB"). The "PERCENTAGE"
  // placeholder will be replaced by the percent savings (e.g. "50%").
  return _("Losslessly compressing %(URL)s could save %(SIZE_IN_BYTES)s "
           "(%(PERCENTAGE)s reduction).");
}

UserFacingString ImageMinifier::child_format_post_gzip() const {
  // None of the image types this rule currently handles are compressible
  // (although there are a few other image types that are, such as SVG), so
  // let's not bother translating a custom string for this thing that shouldn't
  // be happening.
  return child_format();
}

const MinifierOutput* ImageMinifier::Minify(const Resource& resource,
                                            const RuleInput& input) const {
  if (resource.GetResourceType() != IMAGE) {
    return MinifierOutput::CannotBeMinified();
  }

  const ImageType type = resource.GetImageType();
  const std::string& original = resource.GetResponseBody();

  std::string compressed;
  std::string output_mime_type;
  if (type == JPEG) {
    if (!image_compression::OptimizeJpeg(original, &compressed)) {
      DLOG(INFO) << "OptimizeJpeg failed for resource: "
                 << resource.GetRequestUrl();
      return MinifierOutput::Error();
    }
    output_mime_type = "image/jpeg";
  } else if (type == PNG) {
    image_compression::PngReader reader;
    if (!image_compression::PngOptimizer::OptimizePng(reader,
                                                      original,
                                                      &compressed)) {
      DLOG(INFO) << "OptimizePng(PngReader) failed for resource: "
                 << resource.GetRequestUrl();
      return MinifierOutput::Error();
    }
    output_mime_type = "image/png";
  } else if (type == GIF) {
    image_compression::GifReader reader;
    if (!image_compression::PngOptimizer::OptimizePng(reader,
                                                      original,
                                                      &compressed)) {
      DLOG(INFO) << "OptimizePng(GifReader) failed for resource: "
                 << resource.GetRequestUrl();
      return MinifierOutput::Error();
    }
    output_mime_type = "image/png";
  } else {
    return MinifierOutput::CannotBeMinified();
  }

  if (save_optimized_content_) {
    return MinifierOutput::SaveMinifiedContent(compressed, output_mime_type);
  } else {
    return MinifierOutput::DoNotSaveMinifiedContent(compressed);
  }
}

}  // namespace

OptimizeImages::OptimizeImages(bool save_optimized_content)
    : MinifyRule(new ImageMinifier(save_optimized_content)) {}

int OptimizeImages::ComputeScore(const InputInformation& input_info,
                                 const RuleResults& results) {
  WeightedCostBasedScoreComputer score_computer(
      &results, input_info.image_response_bytes(), kCostWeight);
  return score_computer.ComputeScore();
}

}  // namespace rules

}  // namespace pagespeed
