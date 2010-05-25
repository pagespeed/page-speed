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
#include "pagespeed/core/pagespeed_input.h"
#include "pagespeed/core/resource.h"
#include "pagespeed/core/result_provider.h"
#include "pagespeed/proto/pagespeed_output.pb.h"

namespace {

class ImageData {
 public:
  ImageData(const std::string& url,
            int natural_width, int natural_height,
            int client_width, int client_height)
      : url_(url), size_mismatch_(false),
        natural_width_(natural_width), natural_height_(natural_height),
        client_width_(client_width), client_height_(client_height) {
    DCHECK(natural_width >= 0);
    DCHECK(natural_height >= 0);
    DCHECK(client_width >= 0);
    DCHECK(client_height >= 0);
  }

  const std::string& url() const { return url_; }

  double GetCompressionFactor() const;

  bool IsScalable() const;

  void Update(int natural_width, int natural_height,
              int client_width, int client_height);

  int natural_width() const { return natural_width_; }
  int natural_height() const { return natural_height_; }
  int client_width() const { return client_width_; }
  int client_height() const { return client_height_; }

 private:
  std::string url_;
  bool size_mismatch_;
  int natural_width_;
  int natural_height_;
  int client_width_;
  int client_height_;

  DISALLOW_COPY_AND_ASSIGN(ImageData);
};

double ImageData::GetCompressionFactor() const {
  double factor = 1.0;
  if (IsScalable()) {
    if (client_width_ < natural_width_) {
      factor *= (static_cast<double>(client_width_) /
                 static_cast<double>(natural_width_));
    }
    if (client_height_ < natural_height_) {
      factor *= (static_cast<double>(client_height_) /
                 static_cast<double>(natural_height_));
    }
  }
  return factor;
}

bool ImageData::IsScalable() const {
  return (!size_mismatch_ &&
          (client_width_ < natural_width_ ||
           client_height_ < natural_height_));
}

void ImageData::Update(int natural_width, int natural_height,
                       int client_width, int client_height) {
  DCHECK(natural_width >= 0);
  DCHECK(natural_height >= 0);
  DCHECK(client_width >= 0);
  DCHECK(client_height >= 0);

  if (natural_width != natural_width_ ||
      natural_height != natural_height_) {
    LOG(ERROR) << "Mismatched width/height parameters while processing "
               << url_ << ".  Got "
               << natural_width << "x" << natural_height << ", expected "
               << natural_width_ << "x" << natural_height_ << ".";
    size_mismatch_ = true;
    return;
  }

  client_width_ = std::min(std::max(client_width_, client_width),
                           natural_width);
  client_height_ = std::min(std::max(client_height_, client_height),
                            natural_height);
}

typedef std::map<std::string, ImageData*> ImageDataMap;

class ScaledImagesChecker : public pagespeed::DomElementVisitor {
 public:
  // Ownership of document and image_data_map are _not_ transfered to the
  // ScaledImagesChecker.
  ScaledImagesChecker(const pagespeed::PagespeedInput* pagespeed_input,
                      const pagespeed::DomDocument* document,
                      ImageDataMap* image_data_map)
      : pagespeed_input_(pagespeed_input), document_(document),
        image_data_map_(image_data_map) {}

  virtual void Visit(const pagespeed::DomElement& node);

 private:
  const pagespeed::PagespeedInput* pagespeed_input_;
  const pagespeed::DomDocument* document_;
  ImageDataMap* image_data_map_;

  DISALLOW_COPY_AND_ASSIGN(ScaledImagesChecker);
};

void ScaledImagesChecker::Visit(const pagespeed::DomElement& node) {
  if (node.GetTagName() == "IMG") {
    if (pagespeed_input_->has_resource_with_url(document_->GetDocumentUrl())) {
      std::string src;
      int natural_width = 0, natural_height = 0,
          client_width = 0, client_height = 0;
      if (node.GetAttributeByName("src", &src) &&
          node.GetIntPropertyByName("naturalWidth", &natural_width) &&
          node.GetIntPropertyByName("naturalHeight", &natural_height) &&
          node.GetIntPropertyByName("clientWidth", &client_width) &&
          node.GetIntPropertyByName("clientHeight", &client_height)) {
        const std::string url(document_->ResolveUri(src));
        ImageDataMap::iterator iter = image_data_map_->find(url);
        if (iter == image_data_map_->end()) {
          // Ownership of ImageData is transfered to the ImageDataMap.
          (*image_data_map_)[url] =
              new ImageData(url, natural_width, natural_height,
                            client_width, client_height);
        } else {
          iter->second->Update(natural_width, natural_height,
                               client_width, client_height);
        }
      }
    }
  } else if (node.GetTagName() == "IFRAME") {
    // Do a recursive document traversal.
    scoped_ptr<pagespeed::DomDocument> child_doc(node.GetContentDocument());
    if (child_doc.get()) {
      ScaledImagesChecker checker(pagespeed_input_, child_doc.get(),
                                  image_data_map_);
      child_doc->Traverse(&checker);
    }
  }
}

}  // namespace

namespace pagespeed {

namespace rules {

ServeScaledImages::ServeScaledImages() {}

const char* ServeScaledImages::name() const {
  return "ServeScaledImages";
}

const char* ServeScaledImages::header() const {
  return "Serve scaled images";
}

const char* ServeScaledImages::documentation_url() const {
  return "payload.html#ScaleImages";
}

bool ServeScaledImages::AppendResults(const PagespeedInput& input,
                                      ResultProvider* provider) {
  // TODO Consider adding the ability to perform the resizing and provide
  //      the resized image file to the user.
  // TODO Add info about natural/client sizes for use in FormatResults.

  const DomDocument* document = input.dom_document();
  if (!document) {
    return true;
  }

  bool ok = true;
  ImageDataMap image_data_map;
  ScaledImagesChecker visitor(&input, document, &image_data_map);
  document->Traverse(&visitor);

  typedef std::map<const std::string, int> OriginalSizesMap;
  OriginalSizesMap original_sizes_map;
  for (int idx = 0, num = input.num_resources(); idx < num; ++idx) {
    const Resource& resource = input.GetResource(idx);
    original_sizes_map[resource.GetRequestUrl()] =
        resource.GetResponseBody().size();
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
      LOG(ERROR) << "No resource for url: " << url;
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
    image_details->set_expected_height(image_data->natural_height());
    image_details->set_expected_width(image_data->natural_width());
    image_details->set_actual_height(image_data->client_height());
    image_details->set_actual_width(image_data->client_width());
  }

  STLDeleteContainerPairSecondPointers(image_data_map.begin(),
                                       image_data_map.end());

  return ok;
}

void ServeScaledImages::FormatResults(const ResultVector& results,
                                      Formatter* formatter) {
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

  Argument size_arg(Argument::BYTES, total_bytes_saved);
  Argument percent_arg(Argument::INTEGER,
                       (total_original_size == 0 ? 0 :
                        (100 * total_bytes_saved) / total_original_size));
  Formatter* body = formatter->AddChild("The following images are resized in "
                                        "HTML or CSS.  Serving scaled images "
                                        "could save $1 ($2% reduction).",
                                        size_arg, percent_arg);

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
    Argument url_arg(Argument::URL, result.resource_urls(0));
    Argument size_arg(Argument::BYTES, bytes_saved);
    Argument percent_arg(Argument::INTEGER,
                         (original_size == 0 ? 0 :
                          (100 * bytes_saved) / original_size));

    const ResultDetails& details = result.details();
    if (details.HasExtension(ImageDimensionDetails::message_set_extension)) {
      const ImageDimensionDetails& image_details = details.GetExtension(
          ImageDimensionDetails::message_set_extension);
      Argument expected_w(Argument::INTEGER, image_details.expected_width());
      Argument expected_h(Argument::INTEGER, image_details.expected_height());
      Argument actual_w(Argument::INTEGER, image_details.actual_width());
      Argument actual_h(Argument::INTEGER, image_details.actual_height());

      std::string format_str =
          "$1 is resized in HTML or CSS from $2x$3 to $4x$5.  "
          "Serving a scaled image could save $6 ($7% reduction).";
      std::vector<const Argument*> args;
      args.push_back(&url_arg);
      args.push_back(&expected_w);
      args.push_back(&expected_h);
      args.push_back(&actual_w);
      args.push_back(&actual_h);
      args.push_back(&size_arg);
      args.push_back(&percent_arg);

      FormatterParameters formatter_args(&format_str, &args);
      body->AddChild(formatter_args);
    } else {
      body->AddChild("$1 is resized in HTML or CSS.  Serving a "
                     "scaled image could save $2 ($3% reduction).",
                     url_arg, size_arg, percent_arg);
    }
  }
}

}  // namespace rules

}  // namespace pagespeed
