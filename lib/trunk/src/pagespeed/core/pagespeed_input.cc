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

#include "pagespeed/core/pagespeed_input.h"

#include "pagespeed/proto/pagespeed_output.pb.h"

#include "base/logging.h"
#include "base/stl_util-inl.h"
#include "pagespeed/core/dom.h"
#include "pagespeed/core/image_attributes.h"
#include "pagespeed/core/resource.h"
#include "pagespeed/core/resource_util.h"

namespace pagespeed {

PagespeedInput::PagespeedInput()
    : input_info_(new InputInformation),
      resource_filter_(new AllowAllResourceFilter),
      allow_duplicate_resources_(false) {
}

PagespeedInput::PagespeedInput(ResourceFilter* resource_filter)
    : input_info_(new InputInformation),
      resource_filter_(resource_filter),
      allow_duplicate_resources_(false) {
  DCHECK_NE(resource_filter, static_cast<ResourceFilter*>(NULL));
}

PagespeedInput::~PagespeedInput() {
  STLDeleteContainerPointers(resources_.begin(), resources_.end());
}

bool PagespeedInput::IsValidResource(const Resource* resource) const {
  const std::string& url = resource->GetRequestUrl();
  if (url.empty()) {
    LOG(WARNING) << "Refusing Resource with empty URL.";
    return false;
  }
  if (!allow_duplicate_resources_ && has_resource_with_url(url)) {
    LOG(WARNING) << "Ignoring duplicate AddResource for resource at \""
                 << url << "\".";
    return false;
  }
  if (resource->GetResponseStatusCode() <= 0) {
    LOG(WARNING) << "Refusing Resource with invalid status code \""
                 << resource->GetResponseStatusCode() << "\".";
    return false;
  }

  if (resource_filter_.get() && !resource_filter_->IsAccepted(*resource)) {
    return false;
  }

  // TODO: consider adding some basic validation for request/response
  // headers.

  return true;
}

bool PagespeedInput::AddResource(const Resource* resource) {
  if (!IsValidResource(resource)) {
    delete resource;  // Resource is owned by PagespeedInput.
    return false;
  }
  const std::string& url = resource->GetRequestUrl();

  resources_.push_back(resource);
  url_resource_map_[url] = resource;
  host_resource_map_[resource->GetHost()].push_back(resource);

  // Update input information
  int request_bytes = resource_util::EstimateRequestBytes(*resource);
  input_info_->set_total_request_bytes(
      input_info_->total_request_bytes() + request_bytes);
  int response_bytes = resource_util::EstimateResponseBytes(*resource);
  switch (resource->GetResourceType()) {
    case HTML:
      input_info_->set_html_response_bytes(
          input_info_->html_response_bytes() + response_bytes);
      break;
    case TEXT:
      input_info_->set_text_response_bytes(
          input_info_->text_response_bytes() + response_bytes);
      break;
    case CSS:
      input_info_->set_css_response_bytes(
          input_info_->css_response_bytes() + response_bytes);
      break;
    case IMAGE:
      input_info_->set_image_response_bytes(
          input_info_->image_response_bytes() + response_bytes);
      break;
    case JS:
      input_info_->set_javascript_response_bytes(
          input_info_->javascript_response_bytes() + response_bytes);
      break;
    case FLASH:
      input_info_->set_flash_response_bytes(
          input_info_->flash_response_bytes() + response_bytes);
      break;
    case REDIRECT:
    case OTHER:
      input_info_->set_other_response_bytes(
          input_info_->other_response_bytes() + response_bytes);
      break;
    default:
      LOG(DFATAL) << "Unknown resource type " << resource->GetResourceType();
      input_info_->set_other_response_bytes(
          input_info_->other_response_bytes() + response_bytes);
      break;
  }
  input_info_->set_number_resources(num_resources());
  input_info_->set_number_hosts(GetHostResourceMap()->size());
  if (resource_util::IsLikelyStaticResource(*resource)) {
    input_info_->set_number_static_resources(
        input_info_->number_static_resources() + 1);
  }

  return true;
}

bool PagespeedInput::SetPrimaryResourceUrl(const std::string& url) {
  if (!has_resource_with_url(url)) {
    LOG(INFO) << "No such primary resource " << url;
    return false;
  }
  primary_resource_url_ = url;
  return true;
}

void PagespeedInput::AcquireDomDocument(DomDocument* document) {
  document_.reset(document);
}

void PagespeedInput::AcquireImageAttributesFactory(
    ImageAttributesFactory *factory) {
  image_attributes_factory_.reset(factory);
}

int PagespeedInput::num_resources() const {
  return resources_.size();
}

bool PagespeedInput::has_resource_with_url(const std::string& url) const {
  return url_resource_map_.find(url) != url_resource_map_.end();
}

const Resource& PagespeedInput::GetResource(int idx) const {
  DCHECK(idx >= 0 && idx < resources_.size());
  return *resources_[idx];
}

ImageAttributes* PagespeedInput::NewImageAttributes(
    const Resource* resource) const {
  if (image_attributes_factory_ == NULL) {
    return NULL;
  }
  image_attributes_factory_->NewImageAttributes(resource);
}

const HostResourceMap* PagespeedInput::GetHostResourceMap() const {
  return &host_resource_map_;
}

const InputInformation* PagespeedInput::input_information() const {
  return input_info_.get();
}

const DomDocument* PagespeedInput::dom_document() const {
  return document_.get();
}

const std::string& PagespeedInput::primary_resource_url() const {
  return primary_resource_url_;
}

const Resource* PagespeedInput::GetResourceWithUrl(
    const std::string& url) const {
  std::map<std::string, const Resource*>::const_iterator it =
      url_resource_map_.find(url);
  if (it == url_resource_map_.end()) {
    return NULL;
  }
  return it->second;
}

}  // namespace pagespeed
