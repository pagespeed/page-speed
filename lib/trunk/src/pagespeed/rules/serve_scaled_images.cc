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

#include "pagespeed/rules/serve_scaled_images.h"

#include <algorithm>  // for max/min
#include <map>

#include "base/logging.h"
#include "base/scoped_ptr.h"
#include "base/stl_util-inl.h"  // for STLDeleteContainerPairSecondPointers
#include "pagespeed/core/dom.h"
#include "pagespeed/core/formatter.h"
#include "pagespeed/core/image_attributes.h"
#include "pagespeed/core/pagespeed_input.h"
#include "pagespeed/core/resource.h"
#include "pagespeed/core/result_provider.h"
#include "pagespeed/core/rule_input.h"
#include "pagespeed/l10n/l10n.h"
#include "pagespeed/proto/pagespeed_output.pb.h"

namespace {

class ImageData {
 public:
  ImageData(const std::string& url,
            int actual_width, int actual_height,
            int client_width, int client_height)
      : url_(url), size_mismatch_(false),
        actual_width_(actual_width), actual_height_(actual_height),
        client_width_(client_width), client_height_(client_height) {
    DCHECK(actual_width >= 0);
    DCHECK(actual_height >= 0);
    DCHECK(client_width >= 0);
    DCHECK(client_height >= 0);
  }

  const std::string& url() const { return url_; }

  double GetCompressionFactor() const;

  bool IsScalable() const;

  void Update(int actual_width, int actual_height,
              int client_width, int client_height);

  int actual_width() const { return actual_width_; }
  int actual_height() const { return actual_height_; }
  int client_width() const { return client_width_; }
  int client_height() const { return client_height_; }

 private:
  std::string url_;
  bool size_mismatch_;
  int actual_width_;
  int actual_height_;
  int client_width_;
  int client_height_;

  DISALLOW_COPY_AND_ASSIGN(ImageData);
};

double ImageData::GetCompressionFactor() const {
  double factor = 1.0;
  if (IsScalable()) {
    if (client_width_ < actual_width_) {
      factor *= (static_cast<double>(client_width_) /
                 static_cast<double>(actual_width_));
    }
    if (client_height_ < actual_height_) {
      factor *= (static_cast<double>(client_height_) /
                 static_cast<double>(actual_height_));
    }
  }
  return factor;
}

bool ImageData::IsScalable() const {
  return (!size_mismatch_ &&
          (client_width_ < actual_width_ ||
           client_height_ < actual_height_));
}

void ImageData::Update(int actual_width, int actual_height,
                       int client_width, int client_height) {
  DCHECK(actual_width >= 0);
  DCHECK(actual_height >= 0);
  DCHECK(client_width >= 0);
  DCHECK(client_height >= 0);

  if (actual_width != actual_width_ ||
      actual_height != actual_height_) {
    LOG(ERROR) << "Mismatched width/height parameters while processing "
               << url_ << ".  Got "
               << actual_width << "x" << actual_height << ", expected "
               << actual_width_ << "x" << actual_height_ << ".";
    size_mismatch_ = true;
    return;
  }

  client_width_ = std::min(std::max(client_width_, client_width),
                           actual_width);
  client_height_ = std::min(std::max(client_height_, client_height),
                            actual_height);
}

typedef std::map<std::string, ImageData*> ImageDataMap;

class ScaledImagesChecker : public pagespeed::DomElementVisitor {
 public:
  // Ownership of document and image_data_map are _not_ transfered to the
  // ScaledImagesChecker.
  ScaledImagesChecker(const pagespeed::RuleInput* rule_input,
                      const pagespeed::DomDocument* document,
                      ImageDataMap* image_data_map)
      : rule_input_(rule_input), document_(document),
        image_data_map_(image_data_map) {}

  virtual void Visit(const pagespeed::DomElement& node);

 private:
  const pagespeed::RuleInput* rule_input_;
  const pagespeed::DomDocument* document_;
  ImageDataMap* image_data_map_;

  DISALLOW_COPY_AND_ASSIGN(ScaledImagesChecker);
};

void ScaledImagesChecker::Visit(const pagespeed::DomElement& node) {
  if (node.GetTagName() == "IMG") {
    if (rule_input_->pagespeed_input().has_resource_with_url(
            document_->GetDocumentUrl())) {
      std::string src;
      if (node.GetAttributeByName("src", &src)) {
        const std::string url(document_->ResolveUri(src));
        const pagespeed::Resource* resource =
            rule_input_->pagespeed_input().GetResourceCollection()
            .GetRedirectRegistry()->GetFinalRedirectTarget(
                rule_input_->pagespeed_input().GetResourceWithUrlOrNull(url));
        if (resource != NULL) {
          scoped_ptr<pagespeed::ImageAttributes> image_attributes(
              rule_input_->pagespeed_input().NewImageAttributes(resource));
          if (image_attributes != NULL) {
            const int actual_width = image_attributes->GetImageWidth();
            const int actual_height = image_attributes->GetImageHeight();
            int client_width = 0, client_height = 0;
            if (node.GetActualWidth(&client_width) ==
                pagespeed::DomElement::SUCCESS &&
                node.GetActualHeight(&client_height) ==
                pagespeed::DomElement::SUCCESS) {
              ImageDataMap::iterator iter = image_data_map_->find(url);
              if (iter == image_data_map_->end()) {
                // Ownership of ImageData is transfered to the ImageDataMap.
                (*image_data_map_)[url] =
                    new ImageData(url, actual_width, actual_height,
                                  client_width, client_height);
              } else {
                iter->second->Update(actual_width, actual_height,
                                     client_width, client_height);
              }
            }
          }
        }
      }
    }
  } else if (node.GetTagName() == "IFRAME") {
    // Do a recursive document traversal.
    scoped_ptr<pagespeed::DomDocument> child_doc(node.GetContentDocument());
    if (child_doc.get()) {
      ScaledImagesChecker checker(rule_input_, child_doc.get(),
                                  image_data_map_);
      child_doc->Traverse(&checker);
    }
  }
}

}  // namespace

namespace pagespeed {

namespace rules {

ServeScaledImages::ServeScaledImages()
    : pagespeed::Rule(pagespeed::InputCapabilities(
        pagespeed::InputCapabilities::DOM |
        pagespeed::InputCapabilities::RESPONSE_BODY)) {}

const char* ServeScaledImages::name() const {
  return "ServeScaledImages";
}

UserFacingString ServeScaledImages::header() const {
  // TRANSLATOR: The name of a Page Speed rule that is triggered when users
  // serve images, then rescale them in HTML or CSS to the final size (it is
  // more efficient to serve the image with the dimensions it will be shown at).
  // This is displayed at the top of a list of rules names that Page Speed
  // generates.
  return _("Serve scaled images");
}

bool ServeScaledImages::AppendResults(const RuleInput& rule_input,
                                      ResultProvider* provider) {
  // TODO Consider adding the ability to perform the resizing and provide
  //      the resized image file to the user.

  const PagespeedInput& input = rule_input.pagespeed_input();
  const DomDocument* document = input.dom_document();
  if (!document) {
    return true;
  }

  bool ok = true;
  ImageDataMap image_data_map;
  ScaledImagesChecker visitor(&rule_input, document, &image_data_map);
  document->Traverse(&visitor);

  typedef std::map<const std::string, int> OriginalSizesMap;
  OriginalSizesMap original_sizes_map;
  for (int idx = 0, num = input.num_resources(); idx < num; ++idx) {
    const Resource& resource = input.GetResource(idx);
    const Resource* target = input.GetResourceCollection().GetRedirectRegistry()
        ->GetFinalRedirectTarget(&resource);
    if (NULL == target) {
      LOG(DFATAL) << "target == NULL";
      continue;
    }
    original_sizes_map[resource.GetRequestUrl()] =
        target->GetResponseBody().size();
  }

  for (ImageDataMap::const_iterator iter = image_data_map.begin(),
           end = image_data_map.end(); iter != end; ++iter) {
    const ImageData* image_data = iter->second;
    if (!image_data->IsScalable()) {
      continue;
    }

    const std::string& url = image_data->url();
    const OriginalSizesMap::const_iterator size_entry =
        original_sizes_map.find(url);
    if (size_entry == original_sizes_map.end()) {
      LOG(INFO) << "No resource for url: " << url;
      continue;
    }

    const int original_size = size_entry->second;
    const int bytes_saved = original_size -
        static_cast<int>(image_data->GetCompressionFactor() *
                         static_cast<double>(original_size));

    Result* result = provider->NewResult();
    result->set_original_response_bytes(original_size);
    result->add_resource_urls(url);

    Savings* savings = result->mutable_savings();
    savings->set_response_bytes_saved(bytes_saved);

    pagespeed::ResultDetails* details = result->mutable_details();
    pagespeed::ImageDimensionDetails* image_details =
        details->MutableExtension(
            pagespeed::ImageDimensionDetails::message_set_extension);
    image_details->set_expected_height(image_data->actual_height());
    image_details->set_expected_width(image_data->actual_width());
    image_details->set_actual_height(image_data->client_height());
    image_details->set_actual_width(image_data->client_width());
  }

  STLDeleteContainerPairSecondPointers(image_data_map.begin(),
                                       image_data_map.end());

  return ok;
}

void ServeScaledImages::FormatResults(const ResultVector& results,
                                      RuleFormatter* formatter) {
  if (results.size() == 0) {
    return;
  }

  int total_original_size = 0;
  int total_bytes_saved = 0;

  for (ResultVector::const_iterator iter = results.begin(),
           end = results.end();
       iter != end;
       ++iter) {
    const Result& result = **iter;
    total_original_size += result.original_response_bytes();
    const Savings& savings = result.savings();
    total_bytes_saved += savings.response_bytes_saved();
  }

  UrlBlockFormatter* body = formatter->AddUrlBlock(
      // TRANSLATOR: A descriptive header at the top of a list of URLs of
      // images that are resized in HTML or CSS.  It describes the problem to
      // the user.  "$1" is a format token that is replaced with the total
      // savings in bytes from serving images in their final size
      // (e.g. "32.5KiB").  "$2" is replaced with the percentage reduction of
      // bytes transferred (e.g. "25%").
      _("The following images are resized in HTML or CSS.  Serving scaled "
        "images could save $1 ($2 reduction)."),
      BytesArgument(total_bytes_saved),
      PercentageArgument(total_bytes_saved, total_original_size));

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

    const int bytes_saved = result.savings().response_bytes_saved();
    const int original_size = result.original_response_bytes();

    const ResultDetails& details = result.details();
    if (details.HasExtension(ImageDimensionDetails::message_set_extension)) {
      const ImageDimensionDetails& image_details = details.GetExtension(
          ImageDimensionDetails::message_set_extension);
      body->AddUrlResult(
      // TRANSLATOR: Describes a single URL of an image that is resized in HTML
      // or CSS.  It gives the served size of the image, the final size of the
      // image, and the amount saved by serving the image in the final size.
      // "$1" is a format token that will be replaced with the URL of the image
      // resource.  "$2" and "$3" will be replaced with the original (served)
      // width and height (respectively) of the image resource.  "$4" and "$5"
      // will be replaced with the final (resized) width and height
      // (respectively) of the image resource.  "$6" will be replaced with the
      // amount saved (in bytes) by serving the image correctly size (e.g.
      // "32.5KiB").  "$7" will be replaced with the percentage saved (e.g.
      // "25%").
          _("$1 is resized in HTML or CSS from $2x$3 to $4x$5.  "
            "Serving a scaled image could save $6 ($7 reduction)."),
          UrlArgument(result.resource_urls(0)),
          IntArgument(image_details.expected_width()),
          IntArgument(image_details.expected_height()),
          IntArgument(image_details.actual_width()),
          IntArgument(image_details.actual_height()),
          BytesArgument(bytes_saved),
          PercentageArgument(bytes_saved, original_size));
    } else {
      // TRANSLATOR: Describes a single URL of an image that is resized in HTML
      // or CSS.  It gives the amount saved by serving the image in its final
      // size.  "$1" is a format token that will be replaced with the URL of the
      // image resource.  "$2" will be replaced with the amount saved (in bytes)
      // by serving the image correctly size (e.g. "32.5KiB").  "$3" will be
      // replaced with the percentage saved (e.g. "25%").
      body->AddUrlResult(_("$1 is resized in HTML or CSS.  Serving a "
                           "scaled image could save $2 ($3 reduction)."),
                         UrlArgument(result.resource_urls(0)),
                         BytesArgument(bytes_saved),
                         PercentageArgument(bytes_saved, original_size));
    }
  }
}

}  // namespace rules

}  // namespace pagespeed
