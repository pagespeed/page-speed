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

#include "pagespeed/core/resource_collection.h"

#include <algorithm>

#include "base/logging.h"
#include "base/stl_util-inl.h"
#include "pagespeed/core/resource.h"
#include "pagespeed/core/resource_filter.h"
#include "pagespeed/core/uri_util.h"

namespace pagespeed {

namespace {

// sorts resources by their request start times.
struct ResourceRequestStartTimeLessThan {
  bool operator() (const Resource* lhs, const Resource* rhs) const {
    return lhs->IsRequestStartTimeLessThan(*rhs);
  }
};

}  // namespace

bool ResourceUrlLessThan::operator()(
    const Resource* lhs, const Resource* rhs) const {
  return lhs->GetRequestUrl() < rhs->GetRequestUrl();
}

ResourceCollection::ResourceCollection()
    : resource_filter_(new AllowAllResourceFilter),
      frozen_(false) {
}

ResourceCollection::ResourceCollection(ResourceFilter* resource_filter)
    : resource_filter_(resource_filter),
      frozen_(false) {
  DCHECK_NE(resource_filter, static_cast<ResourceFilter*>(NULL));
}

ResourceCollection::~ResourceCollection() {
  STLDeleteContainerPointers(resources_.begin(), resources_.end());
}

bool ResourceCollection::IsValidResource(const Resource* resource) const {
  const std::string& url = resource->GetRequestUrl();
  if (url.empty()) {
    LOG(WARNING) << "Refusing Resource with empty URL.";
    return false;
  }
  if (has_resource_with_url(url)) {
    LOG(INFO) << "Ignoring duplicate AddResource for resource at \""
              << url << "\".";
    return false;
  }
  if (resource->GetResponseStatusCode() <= 0) {
    LOG(WARNING) << "Refusing Resource with invalid status code "
                 << resource->GetResponseStatusCode() << ": " << url;
    return false;
  }

  if (resource_filter_.get() && !resource_filter_->IsAccepted(*resource)) {
    return false;
  }

  // TODO(bmcquade): consider adding some basic validation for
  // request/response headers.

  return true;
}

bool ResourceCollection::AddResource(Resource* resource) {
  if (is_frozen()) {
    LOG(DFATAL) << "Can't add resource " << resource->GetRequestUrl()
                << " to frozen ResourceCollection.";
    delete resource;  // Resource is owned by ResourceCollection.
    return false;
  }
  if (!IsValidResource(resource)) {
    delete resource;  // Resource is owned by ResourceCollection.
    return false;
  }
  const std::string& url = resource->GetRequestUrl();

  resources_.push_back(resource);
  url_resource_map_[url] = resource;
  host_resource_map_[resource->GetHost()].insert(resource);
  return true;
}

bool ResourceCollection::Freeze() {
  bool have_start_times_for_all_resources = true;
  for (int idx = 0, num = num_resources(); idx < num; ++idx) {
    const Resource& resource = GetResource(idx);
    if (!resource.has_request_start_time_millis()) {
      have_start_times_for_all_resources = false;
      break;
    }
  }
  if (have_start_times_for_all_resources) {
    request_order_vector_.assign(resources_.begin(), resources_.end());
    std::stable_sort(request_order_vector_.begin(),
                     request_order_vector_.end(),
                     ResourceRequestStartTimeLessThan());
  }
  frozen_ = true;
  return true;
}

int ResourceCollection::num_resources() const {
  return resources_.size();
}

bool ResourceCollection::has_resource_with_url(const std::string& url) const {
  std::string url_canon;
  if (!uri_util::GetUriWithoutFragment(url, &url_canon)) {
    url_canon = url;
  }
  return url_resource_map_.find(url_canon) != url_resource_map_.end();
}

const Resource& ResourceCollection::GetResource(int idx) const {
  DCHECK(idx >= 0 && static_cast<size_t>(idx) < resources_.size());
  return *resources_[idx];
}

const HostResourceMap* ResourceCollection::GetHostResourceMap() const {
  DCHECK(is_frozen());
  return &host_resource_map_;
}

const ResourceVector*
ResourceCollection::GetResourcesInRequestOrder() const {
  DCHECK(is_frozen());
  if (request_order_vector_.empty()) return NULL;
  DCHECK(request_order_vector_.size() == resources_.size());
  return &request_order_vector_;
}

bool ResourceCollection::is_frozen() const {
  return frozen_;
}

const Resource* ResourceCollection::GetResourceWithUrlOrNull(
    const std::string& url) const {
  std::string url_canon;
  if (!uri_util::GetUriWithoutFragment(url, &url_canon)) {
    url_canon = url;
  }
  std::map<std::string, const Resource*>::const_iterator it =
      url_resource_map_.find(url_canon);
  if (it == url_resource_map_.end()) {
    return NULL;
  }
  if (url_canon != url) {
    LOG(INFO) << "GetResourceWithUrlOrNull(\"" << url
              << "\"): Returning resource with URL " << url_canon;
  }
  return it->second;
}

Resource* ResourceCollection::GetMutableResource(int idx) {
  if (is_frozen()) {
    LOG(DFATAL) << "Unable to get mutable resource after freezing.";
    return NULL;
  }
  return const_cast<Resource*>(&GetResource(idx));
}

Resource* ResourceCollection::GetMutableResourceWithUrlOrNull(
    const std::string& url) {
  if (is_frozen()) {
    LOG(DFATAL) << "Unable to get mutable resource after freezing.";
    return NULL;
  }
  return const_cast<Resource*>(GetResourceWithUrlOrNull(url));
}

}  // namespace pagespeed
