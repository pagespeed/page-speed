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

#include "pagespeed/rules/minify_html.h"

#include <string>

#include "base/logging.h"
#include "pagespeed/core/resource.h"
#include "pagespeed/core/rule_input.h"
#include "pagespeed/html/html_minifier.h"
#include "pagespeed/l10n/l10n.h"
#include "pagespeed/proto/pagespeed_output.pb.h"

namespace pagespeed {

namespace rules {

namespace {

// This cost weight yields an avg score of 83 and a median score of 84
// for the top 100 websites.
const double kCostWeight = 1.5;

class HtmlMinifier : public Minifier {
 public:
  explicit HtmlMinifier(bool save_optimized_content)
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

  DISALLOW_COPY_AND_ASSIGN(HtmlMinifier);
};

const char* HtmlMinifier::name() const {
  return "MinifyHTML";
}

UserFacingString HtmlMinifier::header_format() const {
 // TRANSLATOR: Name of a Page Speed rule. Here, minify means "remove
  // whitespace and comments". The goal is to reduce the size of the
  // HTML file by removing the parts that are unnecessary.
  return _("Minify HTML");
}

UserFacingString HtmlMinifier::body_format() const {
  // TRANSLATOR: Heading that describes the savings possible from minifying
  // resources. The "SIZE_IN_BYTES" placeholder will be replaced by the
  // absolute number of bytes or kilobytes that can be saved (e.g. "5 bytes" or
  // "23.2KiB"). The "PERCENTAGE" placeholder will be replaced by the percent
  // savings (e.g. "50%").
  return _("Minifying the following HTML resources could reduce their "
           "size by %(SIZE_IN_BYTES)s (%(PERCENTAGE)s reduction).");
}

UserFacingString HtmlMinifier::child_format() const {
  // TRANSLATOR: Subheading that describes the savings possible from minifying
  // a single resource.  The "SIZE_IN_BYTES" placeholder will be replaced by
  // the absolute number of bytes or kilobytes that can be saved (e.g. "5
  // bytes" or "23.2KiB"). The "PERCENTAGE" placeholder will be replaced by the
  // percent savings (e.g. "50%").
  return _("Minifying %(URL)s could save %(SIZE_IN_BYTES)s (%(PERCENTAGE)s "
           "reduction).");
}

UserFacingString HtmlMinifier::child_format_post_gzip() const {
  // TRANSLATOR: Subheading that describes the post-compression network savings
  // possible from minifying a single resource.  The "SIZE_IN_BYTES"
  // placeholder will be replaced by the absolute number of bytes or kilobytes
  // that can be saved (e.g. "5 bytes" or "23.2KiB"). The "PERCENTAGE"
  // placeholder will be replaced by the percent savings (e.g. "50%").
  return _("Minifying %(URL)s could save %(SIZE_IN_BYTES)s (%(PERCENTAGE)s "
           "reduction) after compression.");
}

const MinifierOutput* HtmlMinifier::Minify(const Resource& resource,
                                           const RuleInput& rule_input) const {
  if (resource.GetResourceType() != HTML) {
    return MinifierOutput::CannotBeMinified();
  }

  const std::string& content_type = resource.GetResponseHeader("Content-Type");
  const std::string& input = resource.GetResponseBody();
  std::string minified_html;
  ::pagespeed::html::HtmlMinifier html_minifier;
  if (!html_minifier.MinifyHtmlWithType(resource.GetRequestUrl(), content_type,
                                        input, &minified_html)) {
    LOG(ERROR) << "MinifyHtml failed for resource: "
               << resource.GetRequestUrl();
    return MinifierOutput::Error();
  }

  if (save_optimized_content_ && !content_type.empty()) {
    return MinifierOutput::SaveMinifiedContent(minified_html, content_type);
  } else {
    return MinifierOutput::DoNotSaveMinifiedContent(minified_html);
  }
};

}  // namespace

MinifyHTML::MinifyHTML(bool save_optimized_content)
    : MinifyRule(new HtmlMinifier(save_optimized_content)) {}

int MinifyHTML::ComputeScore(const InputInformation& input_info,
                             const RuleResults& results) {
  WeightedCostBasedScoreComputer score_computer(
      &results, input_info.html_response_bytes(), kCostWeight);
  return score_computer.ComputeScore();
}

}  // namespace rules

}  // namespace pagespeed
