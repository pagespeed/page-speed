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

#include "base/logging.h"
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

        pagespeed::Savings* savings = result->mutable_savings();
        savings->set_page_reflows_saved(1);

        int natural_height = 0, natural_width = 0;
        if (node.GetIntPropertyByName("naturalHeight", &natural_height) &&
            node.GetIntPropertyByName("naturalWidth", &natural_width)) {
          pagespeed::ResultDetails* details = result->mutable_details();
          pagespeed::ImageDimensionDetails* image_details =
              details->MutableExtension(
                  pagespeed::ImageDimensionDetails::message_set_extension);
          image_details->set_expected_height(natural_height);
          image_details->set_expected_width(natural_width);
        }
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
  if (results.empty()) {
    return;
  }

  Formatter* body = formatter->AddChild(
      "The following image(s) are missing width and/or height attributes.");

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

    const ResultDetails& details = result.details();
    if (details.HasExtension(ImageDimensionDetails::message_set_extension)) {
      const ImageDimensionDetails& image_details = details.GetExtension(
          ImageDimensionDetails::message_set_extension);

      Argument url(Argument::URL, result.resource_urls(0));
      Argument height(Argument::INTEGER, image_details.expected_height());
      Argument width(Argument::INTEGER, image_details.expected_width());
      body->AddChild("$1 (Dimensions: $2 x $3)", url, height, width);
    } else {
      Argument url(Argument::URL, result.resource_urls(0));
      body->AddChild("$1", url);
    }
  }
}

}  // namespace rules

}  // namespace pagespeed
