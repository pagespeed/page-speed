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
#include "pagespeed/core/resource.h"
#include "pagespeed/core/resource_util.h"

namespace pagespeed {

PagespeedInput::PagespeedInput()
    : allow_duplicate_resources_(false),
      input_info_(new InputInformation),
      resource_filter_(new AllowAllResourceFilter) {
}

PagespeedInput::PagespeedInput(ResourceFilter* resource_filter)
    : allow_duplicate_resources_(false),
      input_info_(new InputInformation),
      resource_filter_(resource_filter) {
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
  if (!allow_duplicate_resources_ &&
      resource_urls_.find(url) != resource_urls_.end()) {
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
  resource_urls_.insert(url);
  host_resource_map_[resource->GetHost()].push_back(resource);

  // Update input information
  int request_bytes = resource_util::EstimateRequestBytes(*resource);
  input_info_->set_total_request_bytes(
      input_info_->total_request_bytes() + request_bytes);
  int response_bytes = resource_util::EstimateResponseBytes(*resource);
  input_info_->set_total_response_bytes(
      input_info_->total_response_bytes() + response_bytes);
  input_info_->set_number_resources(num_resources());
  input_info_->set_number_hosts(GetHostResourceMap()->size());

  return true;
}

void PagespeedInput::AcquireDomDocument(DomDocument* document) {
  document_.reset(document);
}

int PagespeedInput::num_resources() const {
  return resources_.size();
}

const Resource& PagespeedInput::GetResource(int idx) const {
  DCHECK(idx >= 0 && idx < resources_.size());
  return *resources_[idx];
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

}  // namespace pagespeed
