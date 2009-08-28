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

#include "base/logging.h"
#include "base/stl_util-inl.h"
#include "pagespeed/core/resource.h"

namespace pagespeed {

PagespeedInput::PagespeedInput() {
}

PagespeedInput::~PagespeedInput() {
  STLDeleteContainerPointers(resources_.begin(), resources_.end());
}

void PagespeedInput::AddResource(const Resource* resource) {
  resources_.push_back(resource);
  host_resource_map_[resource->GetHost()].push_back(resource);
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

}  // namespace pagespeed
