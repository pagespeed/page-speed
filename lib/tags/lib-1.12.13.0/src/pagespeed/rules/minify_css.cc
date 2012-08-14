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

#include "pagespeed/rules/minify_css.h"

#include <string>

#include "base/logging.h"
#include "pagespeed/core/resource.h"
#include "pagespeed/core/resource_util.h"
#include "pagespeed/core/rule_input.h"
#include "pagespeed/css/cssmin.h"
#include "pagespeed/l10n/l10n.h"
#include "pagespeed/proto/pagespeed_output.pb.h"

namespace pagespeed {

namespace rules {

namespace {

// This cost weight yields an avg score of 83 and a median score of 100
// for the top 100 websites.
const double kCostWeight = 3.5;

class CssMinifier : public Minifier {
 public:
  explicit CssMinifier(bool save_optimized_content)
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

  DISALLOW_COPY_AND_ASSIGN(CssMinifier);
};

const char* CssMinifier::name() const {
  return "MinifyCss";
}

UserFacingString CssMinifier::header_format() const {
  // TRANSLATOR: Name of a Page Speed rule. Here, minify means "remove
  // whitespace and comments". The goal is to reduce the size of the
  // CSS file by removing the parts that are unnecessary.
  return _("Minify CSS");
}

UserFacingString CssMinifier::body_format() const {
  // TRANSLATOR: Heading that describes the savings possible from
  // minifying resources. "$1" is a format token that will be replaced by the
  // absolute number of bytes or kilobytes that can be saved (e.g. "5 bytes" or
  // "23.2KiB"). "$2" will be replaced by the percent savings (e.g. "50%").
  return _("Minifying the following CSS resources could "
           "reduce their size by $1 ($2 reduction).");
}

UserFacingString CssMinifier::child_format() const {
  // TRANSLATOR: Subheading that describes the savings possible from minifying a
  // single resource. "$1" is a format token that will be replaced by the URL of
  // the resource. "$2" will be replaced bythe absolute number of bytes or
  // kilobytes that can be saved (e.g. "5 bytes" or "23.2KiB"). "$3" will be
  // replaced by the percent savings (e.g. "50%").
  return _("Minifying $1 could save $2 ($3 reduction).");
}

UserFacingString CssMinifier::child_format_post_gzip() const {
  // TRANSLATOR: Subheading that describes the post-compression network savings
  // possible from minifying a single resource. "$1" is a format token that
  // will be replaced by the URL of the resource. "$2" will be replaced bythe
  // absolute number of bytes or kilobytes that can be saved (e.g. "5 bytes" or
  // "23.2KiB"). "$3" will be replaced by the percent savings (e.g. "50%").
  return _("Minifying $1 could save $2 ($3 reduction) after compression.");
}

const MinifierOutput* CssMinifier::Minify(const Resource& resource,
                                          const RuleInput& rule_input) const {
  if (resource.GetResourceType() != CSS) {
    return MinifierOutput::CannotBeMinified();
  }

  const std::string& input = resource.GetResponseBody();
  if (save_optimized_content_ ||
      resource_util::IsCompressedResource(resource)) {
    std::string minified_css;
    if (!css::MinifyCss(input, &minified_css)) {
      LOG(ERROR) << "MinifyCss failed for resource: "
                 << resource.GetRequestUrl();
      return MinifierOutput::Error();
    }
    if (save_optimized_content_) {
      return MinifierOutput::SaveMinifiedContent(minified_css, "text/css");
    } else {
      return MinifierOutput::DoNotSaveMinifiedContent(minified_css);
    }
  } else {
    int minified_css_size = 0;
    if (!css::GetMinifiedCssSize(input, &minified_css_size)) {
      LOG(ERROR) << "GetMinifiedCssSize failed for resource: "
                 << resource.GetRequestUrl();
      return MinifierOutput::Error();
    }
    return MinifierOutput::PlainMinifiedSize(minified_css_size);
  }
};

}  // namespace

MinifyCss::MinifyCss(bool save_optimized_content)
    : MinifyRule(new CssMinifier(save_optimized_content)) {}

int MinifyCss::ComputeScore(const InputInformation& input_info,
                            const RuleResults& results) {
  WeightedCostBasedScoreComputer score_computer(
      &results, input_info.css_response_bytes(), kCostWeight);
  return score_computer.ComputeScore();
}

}  // namespace rules

}  // namespace pagespeed
