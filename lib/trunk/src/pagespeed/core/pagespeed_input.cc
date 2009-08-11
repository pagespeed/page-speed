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
#include "pagespeed/core/pagespeed_input.pb.h"
#include "pagespeed/core/resource.h"

namespace pagespeed {

PagespeedInput::PagespeedInput(const ProtoInput* input_proto)
    : input_proto_(input_proto) {
  CHECK(input_proto != NULL);

  typedef ::google::protobuf::RepeatedPtrField<pagespeed::ProtoResource>
      ResourceList;

  const ResourceList& serialized_resources = input_proto->resources();
  for (ResourceList::const_iterator iter = serialized_resources.begin(),
           end = serialized_resources.end();
       iter != end;
       ++iter) {
    pagespeed::Resource* resource = new pagespeed::Resource(*iter);
    resources_.push_back(resource);
  }

  for (int idx = 0, num = num_resources(); idx < num; ++idx) {
    const Resource& resource = GetResource(idx);
    host_resource_map_[resource.GetHost()].push_back(&resource);
  }
}

PagespeedInput::~PagespeedInput() {
  STLDeleteContainerPointers(resources_.begin(), resources_.end());
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
