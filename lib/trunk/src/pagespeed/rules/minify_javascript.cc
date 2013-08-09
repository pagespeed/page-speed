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
#include "pagespeed/core/resource.h"
#include "pagespeed/core/resource_util.h"
#include "pagespeed/core/rule_input.h"
#include "pagespeed/l10n/l10n.h"
#include "pagespeed/proto/pagespeed_output.pb.h"
#include "pagespeed/js/js_minify.h"

namespace pagespeed {

namespace rules {

namespace {

// This cost weight yields an avg score of 84 and a median score of 97
// for the top 100 websites.
const double kCostWeight = 3.5;

class JsMinifier : public Minifier {
 public:
  explicit JsMinifier(bool save_optimized_content)
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

  DISALLOW_COPY_AND_ASSIGN(JsMinifier);
};

const char* JsMinifier::name() const {
  return "MinifyJavaScript";
}

UserFacingString JsMinifier::header_format() const {
  // TRANSLATOR: Name of a Page Speed rule. Here, minify means "remove
  // whitespace and comments". The goal is to reduce the size of the
  // JavaScript file by removing the parts that are unnecessary.
  return _("Minify JavaScript");
}

UserFacingString JsMinifier::body_format() const {
  // TRANSLATOR: Heading that describes the savings possible from minifying
  // resources. The "SIZE_IN_BYTES" placeholder will be replaced by the
  // absolute number of bytes or kilobytes that can be saved (e.g. "5 bytes" or
  // "23.2KiB"). The "PERCENTAGE" placeholder will be replaced by the percent
  // savings (e.g. "50%").
  return _("Minifying the following JavaScript resources could reduce their "
           "size by %(SIZE_IN_BYTES)s (%(PERCENTAGE)s reduction).");
}

UserFacingString JsMinifier::child_format() const {
  // TRANSLATOR: Subheading that describes the savings possible from minifying
  // a single resource.  The "SIZE_IN_BYTES" placeholder will be replaced by
  // the absolute number of bytes or kilobytes that can be saved (e.g. "5
  // bytes" or "23.2KiB"). The "PERCENTAGE" placeholder will be replaced by the
  // percent savings (e.g. "50%").
  return _("Minifying %(URL)s could save %(SIZE_IN_BYTES)s (%(PERCENTAGE)s "
           "reduction).");
}

UserFacingString JsMinifier::child_format_post_gzip() const {
  // TRANSLATOR: Subheading that describes the post-compression network savings
  // possible from minifying a single resource.  The "SIZE_IN_BYTES"
  // placeholder will be replaced by the absolute number of bytes or kilobytes
  // that can be saved (e.g. "5 bytes" or "23.2KiB"). The "PERCENTAGE"
  // placeholder will be replaced by the percent savings (e.g. "50%").
  return _("Minifying %(URL)s could save %(SIZE_IN_BYTES)s (%(PERCENTAGE)s "
           "reduction) after compression.");
}

const MinifierOutput* JsMinifier::Minify(const Resource& resource,
                                         const RuleInput& rule_input) const {
  if (resource.GetResourceType() != JS) {
    return MinifierOutput::CannotBeMinified();
  }

  const std::string& input = resource.GetResponseBody();
  if (save_optimized_content_ ||
      resource_util::IsCompressedResource(resource)) {
    std::string minified_js;
    if (!js::MinifyJs(input, &minified_js)) {
      LOG(ERROR) << "MinifyJs failed for resource: "
                 << resource.GetRequestUrl();
      return MinifierOutput::Error();
    }
    if (save_optimized_content_) {
      return MinifierOutput::SaveMinifiedContent(minified_js,
                                                 "text/javascript");
    } else {
      return MinifierOutput::DoNotSaveMinifiedContent(minified_js);
    }
  } else {
    int minified_js_size = 0;
    if (!js::GetMinifiedJsSize(input, &minified_js_size)) {
      LOG(ERROR) << "GetMinifiedJsSize failed for resource: "
                 << resource.GetRequestUrl();
      return MinifierOutput::Error();
    }
    return MinifierOutput::PlainMinifiedSize(minified_js_size);
  }
};

}  // namespace

MinifyJavaScript::MinifyJavaScript(bool save_optimized_content)
    : MinifyRule(new JsMinifier(save_optimized_content)) {}

int MinifyJavaScript::ComputeScore(const InputInformation& input_info,
                                   const RuleResults& results) {
  WeightedCostBasedScoreComputer score_computer(
      &results, input_info.javascript_response_bytes(), kCostWeight);
  return score_computer.ComputeScore();
}

}  // namespace rules

}  // namespace pagespeed
