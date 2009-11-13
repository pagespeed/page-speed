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
#include "pagespeed/core/resource.h"

namespace {

// each message header has a 3 byte overhead; colon between the key
// value pair and the end-of-line CRLF.
const int kHeaderOverhead = 3;

}  // namespace

namespace pagespeed {

PagespeedInput::PagespeedInput()
    : allow_duplicate_resources_(false),
      input_info_(new InputInformation) {
}

PagespeedInput::~PagespeedInput() {
  STLDeleteContainerPointers(resources_.begin(), resources_.end());
}

bool PagespeedInput::AddResource(const Resource* resource) {
  const std::string& url = resource->GetRequestUrl();
  if (!allow_duplicate_resources_ &&
      resource_urls_.find(url) != resource_urls_.end()) {
    LOG(WARNING) << "Ignoring duplicate AddResource for resource at \""
                 << url << "\".";
    delete resource;  // Resource is owned by PagespeedInput.
    return false;
  }

  resources_.push_back(resource);
  resource_urls_.insert(url);
  host_resource_map_[resource->GetHost()].push_back(resource);

  // Update input information
  int response_bytes = 0;
  // TODO get compressed size or replace with section with actual
  // download size.
  // TODO improve the header size calculation below.
  response_bytes += resource->GetResponseBody().size();
  response_bytes += resource->GetResponseProtocol().size();
  for (std::map<std::string, std::string>::const_iterator
           iter = resource->GetResponseHeaders()->begin(),
           end = resource->GetResponseHeaders()->end();
       iter != end;
       ++iter) {
    response_bytes += kHeaderOverhead +
        iter->first.size() +
        iter->second.size();
  }

  input_info_->set_total_response_bytes(
      input_info_->total_response_bytes() + response_bytes);
  input_info_->set_number_resources(num_resources());
  input_info_->set_number_hosts(GetHostResourceMap()->size());

  return true;
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

}  // namespace pagespeed
