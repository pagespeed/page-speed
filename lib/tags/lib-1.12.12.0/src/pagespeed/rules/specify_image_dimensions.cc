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

#include <map>

#include "base/basictypes.h"
#include "base/logging.h"
#include "base/scoped_ptr.h"
#include "pagespeed/core/dom.h"
#include "pagespeed/core/formatter.h"
#include "pagespeed/core/image_attributes.h"
#include "pagespeed/core/pagespeed_input.h"
#include "pagespeed/core/resource.h"
#include "pagespeed/core/result_provider.h"
#include "pagespeed/core/rule_input.h"
#include "pagespeed/core/uri_util.h"
#include "pagespeed/l10n/l10n.h"
#include "pagespeed/proto/pagespeed_output.pb.h"

namespace {

const char* kRuleName = "SpecifyImageDimensions";

class ImageDimensionsChecker : public pagespeed::DomElementVisitor {
 public:
  ImageDimensionsChecker(const pagespeed::RuleInput* rule_input,
                         const pagespeed::DomDocument* document,
                         pagespeed::ResultProvider* provider)
      : rule_input_(rule_input), document_(document), provider_(provider) {}

  virtual void Visit(const pagespeed::DomElement& node);

 private:
  const pagespeed::RuleInput* rule_input_;
  const pagespeed::DomDocument* document_;
  pagespeed::ResultProvider* provider_;

  DISALLOW_COPY_AND_ASSIGN(ImageDimensionsChecker);
};

void ImageDimensionsChecker::Visit(const pagespeed::DomElement& node) {
  if (node.GetTagName() == "IMG") {
    if (rule_input_->pagespeed_input().has_resource_with_url(
            document_->GetDocumentUrl())) {
      bool height_specified = false;
      bool width_specified = false;
      if (pagespeed::DomElement::SUCCESS !=
          node.HasHeightSpecified(&height_specified) ||
          pagespeed::DomElement::SUCCESS !=
          node.HasWidthSpecified(&width_specified)) {
        // The runtime was not able to compute the requested values,
        // so we must skip this node.
        return;
      }
      if (!height_specified || !width_specified) {
        std::string src;
        if (!node.GetAttributeByName("src", &src)) {
          return;
        }
        const std::string uri = document_->ResolveUri(src);

        // Don't complain about image tags with non-external resource
        // URIs (e.g. data URIs), because the browser already knows
        // the image dimensions once it has the image data.
        if (!pagespeed::uri_util::IsExternalResourceUrl(uri)) {
          return;
        }

        pagespeed::Result* result = provider_->NewResult();
        result->add_resource_urls(uri);

        pagespeed::Savings* savings = result->mutable_savings();
        savings->set_page_reflows_saved(1);

        const pagespeed::Resource* resource =
            rule_input_->pagespeed_input().GetResourceCollection()
            .GetRedirectRegistry()->GetFinalRedirectTarget(
                rule_input_->pagespeed_input().GetResourceWithUrlOrNull(uri));
        if (resource != NULL) {
          scoped_ptr<pagespeed::ImageAttributes> image_attributes(
              rule_input_->pagespeed_input().NewImageAttributes(resource));
          if (image_attributes != NULL) {
            pagespeed::ResultDetails* details = result->mutable_details();
            pagespeed::ImageDimensionDetails* image_details =
                details->MutableExtension(
                    pagespeed::ImageDimensionDetails::message_set_extension);
            image_details->set_expected_height(
                image_attributes->GetImageHeight());
            image_details->set_expected_width(
                image_attributes->GetImageWidth());
          }
        }
      }
    }
  } else if (node.GetTagName() == "IFRAME") {
    // Do a recursive document traversal.
    scoped_ptr<pagespeed::DomDocument> child_doc(node.GetContentDocument());
    if (child_doc.get()) {
      ImageDimensionsChecker checker(rule_input_, child_doc.get(),
                                     provider_);
      child_doc->Traverse(&checker);
    }
  }
}

// sorts results by their URLs.
struct ResultUrlLessThan {
  bool operator() (const pagespeed::Result& lhs,
                   const pagespeed::Result& rhs) const {
    return lhs.resource_urls(0) < rhs.resource_urls(0);
  }
};

}  // namespace

namespace pagespeed {

namespace rules {

SpecifyImageDimensions::SpecifyImageDimensions()
    : pagespeed::Rule(pagespeed::InputCapabilities(
        pagespeed::InputCapabilities::DOM |
        pagespeed::InputCapabilities::RESPONSE_BODY)) {}

const char* SpecifyImageDimensions::name() const {
  return kRuleName;
}

UserFacingString SpecifyImageDimensions::header() const {
  // TRANSLATOR: The name of a Page Speed rule that tells users to ensure that
  // their webpage explicitly specifies the width/height dimensions of each
  // image that appears in the page.  This is displayed in a list of rule names
  // that Page Speed generates.
  return _("Specify image dimensions");
}

bool SpecifyImageDimensions::AppendResults(const RuleInput& rule_input,
                                           ResultProvider* provider) {
  const DomDocument* document = rule_input.pagespeed_input().dom_document();
  if (document) {
    ImageDimensionsChecker visitor(&rule_input, document, provider);
    document->Traverse(&visitor);
  }
  return true;
}

void SpecifyImageDimensions::FormatResults(const ResultVector& results,
                                           RuleFormatter* formatter) {
  if (results.empty()) {
    return;
  }

  UrlBlockFormatter* body = formatter->AddUrlBlock(
      // TRANSLATOR: Header at the top of a list of URLs of images that Page
      // Speed detected as not having both width and height explicitly
      // specified in the page in which the image appears.
      _("The following image(s) are missing width and/or height attributes."));

  std::map<Result, int, ResultUrlLessThan> result_count_map;
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

    result_count_map[result]++;
  }

  for (std::map<Result, int, ResultUrlLessThan>::const_iterator iter =
          result_count_map.begin();
       iter != result_count_map.end();
       ++iter) {

    const Result& result = iter->first;
    int count = iter->second;

    const ResultDetails& details = result.details();
    if (details.HasExtension(ImageDimensionDetails::message_set_extension)) {
      const ImageDimensionDetails& image_details = details.GetExtension(
          ImageDimensionDetails::message_set_extension);
      if ( count > 1 ) {
        // TRANSLATOR: A format string for one item in a list of images that
        // Page Speed detected as not having both width and height explicitly
        // specified in the page in which the image appears; each list item
        // provides the URL of the image, and the actual width/height
        // dimensions of the image to aid the user in specifying those
        // dimensions in the page.  The "$1" is a format token that will be
        // replaced with the URL of the image; the "$2" is a format token that
        // will be replaced with the width of the image, in pixels
        // (e.g. "320"); the $3 is a format token that will be replaced with
        // the height of the image, in pixels (e.g. "240"); the $4 is a format
        // token that will be replaced with the number of times this image
        // appears in the page (e.g. "3").
        body->AddUrlResult(_("$1 (Dimensions: $2 x $3) ($4 uses)"),
                           UrlArgument(result.resource_urls(0)),
                           IntArgument(image_details.expected_width()),
                           IntArgument(image_details.expected_height()),
                           IntArgument(count));
      } else {
        // TRANSLATOR: A format string for one item in a list of images that
        // Page Speed detected as not having both width and height explicitly
        // specified in the page in which the image appears; each list item
        // provides the URL of the image, and the actual width/height
        // dimensions of the image to aid the user in specifying those
        // dimensions in the page.  The "$1" is a format token that will be
        // replaced with the URL of the image; the "$2" is a format token that
        // will be replaced with the width of the image, in pixels
        // (e.g. "320"); the $3 is a format token that will be replaced with
        // the height of the image, in pixels (e.g. "240").
        body->AddUrlResult(_("$1 (Dimensions: $2 x $3)"),
                           UrlArgument(result.resource_urls(0)),
                           IntArgument(image_details.expected_width()),
                           IntArgument(image_details.expected_height()));
      }
    } else {
      body->AddUrl(result.resource_urls(0));
    }
  }
}

}  // namespace rules

}  // namespace pagespeed
