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

#include "pagespeed/rules/specify_image_dimensions.h"

#include <set>

#include "base/scoped_ptr.h"
#include "pagespeed/core/dom.h"
#include "pagespeed/core/formatter.h"
#include "pagespeed/core/pagespeed_input.h"
#include "pagespeed/proto/pagespeed_output.pb.h"

namespace {

const char* kRuleName = "SpecifyImageDimensions";

class ImageDimensionsChecker : public pagespeed::DomElementVisitor {
 public:
  explicit ImageDimensionsChecker(pagespeed::Results* results)
      : results_(results) {}

  virtual void Visit(const pagespeed::DomElement& node) {
    if (node.GetTagName() == "IMG") {
      std::string height, width;
      bool height_specified = (node.GetAttributeByName("height", &height) ||
                               node.GetCSSPropertyByName("height", &height));
      bool width_specified = (node.GetAttributeByName("width", &width) ||
                              node.GetCSSPropertyByName("width", &width));

      if (!height_specified || !width_specified) {
        std::string src;
        if (!node.GetResourceUrl(&src)) {
          return;
        }
        pagespeed::Result* result = results_->add_results();
        result->set_rule_name(kRuleName);
        result->add_resource_urls(src);
      }
    } else if (node.GetTagName() == "IFRAME") {
      // Do a recursive document traversal.
      scoped_ptr<pagespeed::DomDocument> child_doc(node.GetContentDocument());
      if (child_doc.get()) {
        child_doc->Traverse(this);
      }
    }
  }
 private:
  pagespeed::Results* results_;
};

}  // namespace

namespace pagespeed {

namespace rules {

SpecifyImageDimensions::SpecifyImageDimensions() {}

const char* SpecifyImageDimensions::name() const {
  return kRuleName;
}

const char* SpecifyImageDimensions::header() const {
  return "Specify image dimensions";
}

const char* SpecifyImageDimensions::documentation_url() const {
  return "rendering.html#SpecifyImageDimensions";
}

bool SpecifyImageDimensions::AppendResults(const PagespeedInput& input,
                                           Results* results) {
  if (input.dom_document()) {
    ImageDimensionsChecker visitor(results);
    input.dom_document()->Traverse(&visitor);
  }
  return true;
}

void SpecifyImageDimensions::FormatResults(const ResultVector& results,
                                           Formatter* formatter) {
  std::set<std::string> violation_urls;
  for (ResultVector::const_iterator iter = results.begin(),
           end = results.end();
       iter != end;
       ++iter) {
    const Result& result = **iter;

    for (int idx = 0; idx < result.resource_urls_size(); idx++) {
      violation_urls.insert(result.resource_urls(idx));
    }
  }

  if (violation_urls.empty()) {
    return;
  }

  Formatter* body = formatter->AddChild(
      "The following image(s) are missing width and/or height attributes.");

  for (std::set<std::string>::const_iterator iter = violation_urls.begin(),
           end = violation_urls.end();
       iter != end;
       ++iter) {
    Argument url(Argument::URL, *iter);
    body->AddChild("$1", url);
  }
}

}  // namespace rules

}  // namespace pagespeed
