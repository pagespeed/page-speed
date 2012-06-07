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

#include "pagespeed/core/rule_input.h"

#include "base/logging.h"
#include "pagespeed/core/pagespeed_input.h"
#include "pagespeed/core/resource.h"
#include "pagespeed/core/resource_util.h"

namespace pagespeed {

RuleInput::RuleInput(const PagespeedInput& pagespeed_input)
    : pagespeed_input_(&pagespeed_input),
      initialized_(false) {
  if (!pagespeed_input_->is_frozen()) {
    LOG(DFATAL) << "Passed non-frozen PagespeedInput to RuleInput.";
  }
}

void RuleInput::Init() {
  if (!initialized_) {
    initialized_ = true;
  }
}

bool RuleInput::GetCompressedResponseBodySize(const Resource& resource,
                                              int* output) const {
  // If the compressed size for this resource is already in the map, return
  // that memoized value.
  const std::map<const Resource*, int>::const_iterator iter =
      compressed_response_body_sizes_.find(&resource);
  if (iter != compressed_response_body_sizes_.end()) {
    *output = iter->second;
    return true;
  }

  // Compute the compressed size of the resource (or original size if the
  // resource is not compressible).
  int compressed_size;
  if (::pagespeed::resource_util::IsCompressibleResource(resource) ||
      ::pagespeed::resource_util::IsCompressedResource(resource)) {
    if (!::pagespeed::resource_util::GetGzippedSize(
            resource.GetResponseBody(), &compressed_size)) {
      return false;
    }
  } else {
    compressed_size = resource.GetResponseBody().size();
  }

  // Memoize and return the compressed size.
  compressed_response_body_sizes_[&resource] = compressed_size;
  *output = compressed_size;
  return true;
}

}  // namespace pagespeed
