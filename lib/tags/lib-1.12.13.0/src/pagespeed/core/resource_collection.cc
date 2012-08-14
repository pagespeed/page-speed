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
#include "pagespeed/core/resource_util.h"
#include "pagespeed/core/uri_util.h"

namespace pagespeed {

namespace {

// sorts resources by their request start times.
struct ResourceRequestStartTimeLessThan {
  bool operator() (const Resource* lhs, const Resource* rhs) const {
    return lhs->IsRequestStartTimeLessThan(*rhs);
  }
};

class RedirectGraph {
 public:
  explicit RedirectGraph(const ResourceCollection* resource_collection)
      : resource_collection_(resource_collection) {}
  void AddResource(const Resource& resource);
  void AppendRedirectChainResults(
      RedirectRegistry::RedirectChainVector* chains);

 private:
  // Build a prioritized vector of possible roots.
  // This vector should contain all redirect sources, but give
  // priority to those that are not redirect targets.  We cannot
  // exclude all redirect targets because we would like to warn about
  // pure redirect loops.
  void GetPriorizedRoots(std::vector<std::string>* roots);
  void PopulateRedirectChainResult(const std::string& root,
                                   RedirectRegistry::RedirectChain* chain);

  typedef std::map<std::string, std::vector<std::string> > RedirectMap;
  RedirectMap redirect_map_;
  std::set<std::string> destinations_;
  std::set<std::string> processed_;
  const ResourceCollection* resource_collection_;
};

void RedirectGraph::AddResource(const Resource& resource) {
  std::string destination =
      resource_util::GetRedirectedUrl(resource);
  if (!destination.empty()) {
    redirect_map_[resource.GetRequestUrl()].push_back(destination);
    destinations_.insert(destination);
  }
}

void RedirectGraph::AppendRedirectChainResults(
    RedirectRegistry::RedirectChainVector* chains) {
  std::vector<std::string> roots;
  GetPriorizedRoots(&roots);

  // compute chains
  for (std::vector<std::string>::const_iterator it = roots.begin(),
           end = roots.end();
       it != end;
       ++it) {
    if (processed_.find(*it) != processed_.end()) {
      continue;
    }
    RedirectRegistry::RedirectChain chain;
    chains->push_back(chain);

    PopulateRedirectChainResult(*it, &(chains->back()));
  }
}

void RedirectGraph::GetPriorizedRoots(std::vector<std::string>* roots) {
  std::vector<std::string> primary_roots, secondary_roots;
  for (RedirectMap::const_iterator it = redirect_map_.begin(),
           end = redirect_map_.end();
       it != end;
       ++it) {
    const std::string& root = it->first;
    if (destinations_.find(root) == destinations_.end()) {
      primary_roots.push_back(root);
    } else {
      secondary_roots.push_back(root);
    }
  }
  roots->insert(roots->end(), primary_roots.begin(), primary_roots.end());
  roots->insert(roots->end(), secondary_roots.begin(), secondary_roots.end());
}

void RedirectGraph::PopulateRedirectChainResult(
    const std::string& root, RedirectRegistry::RedirectChain* chain) {
  // Perform a DFS on the redirect graph.
  std::vector<std::string> work_stack;
  work_stack.push_back(root);
  while (!work_stack.empty()) {
    std::string current = work_stack.back();
    work_stack.pop_back();
    const Resource* resource =
        resource_collection_->GetResourceWithUrlOrNull(current);
    if (resource == NULL) {
      LOG(INFO) << "Unable to find resource with URL " << current;
      continue;
    }
    chain->push_back(resource);

    // detect and break loops.
    if (processed_.find(current) != processed_.end()) {
      continue;
    }
    processed_.insert(current);

    // add backwards so direct decendents are traversed in
    // alphabetical order.
    const std::vector<std::string>& targets = redirect_map_[current];
    work_stack.insert(work_stack.end(), targets.rbegin(), targets.rend());
  }
}

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
  redirect_registry_.Init(*this);
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

const RedirectRegistry* ResourceCollection::GetRedirectRegistry() const {
  DCHECK(is_frozen());
  return &redirect_registry_;
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

RedirectRegistry::RedirectRegistry() : initialized_(false) {}

void RedirectRegistry::Init(const ResourceCollection& resource_collection) {
  DCHECK(!initialized_);
  DCHECK(resource_collection.is_frozen());
  if (!initialized_ && resource_collection.is_frozen()) {
    BuildRedirectChains(resource_collection);
    initialized_ = true;
  }
}

void RedirectRegistry::BuildRedirectChains(
    const ResourceCollection& resource_collection) {
  RedirectGraph redirect_graph(&resource_collection);
  for (int idx = 0, num = resource_collection.num_resources();
       idx < num; ++idx) {
    redirect_graph.AddResource(resource_collection.GetResource(idx));
  }

  redirect_chains_.clear();
  redirect_graph.AppendRedirectChainResults(&redirect_chains_);

  // Map resource to chains.
  for (RedirectRegistry::RedirectChainVector::const_iterator it =
       redirect_chains_.begin(), end = redirect_chains_.end();
       it != end;
       ++it) {
    const RedirectRegistry::RedirectChain& chain = *it;
    for (RedirectRegistry::RedirectChain::const_iterator cit = chain.begin(),
        cend = chain.end();
        cit != cend;
        ++cit) {
      resource_to_redirect_chain_map_[*cit] = &chain;
    }
  }
}

const RedirectRegistry::RedirectChainVector&
RedirectRegistry::GetRedirectChains() const {
  DCHECK(initialized_);
  return redirect_chains_;
};

const RedirectRegistry::RedirectChain* RedirectRegistry::GetRedirectChainOrNull(
    const Resource* resource) const {
  DCHECK(initialized_);
  if (resource == NULL) {
    return NULL;
  }
  ResourceToRedirectChainMap::const_iterator it =
      resource_to_redirect_chain_map_.find(resource);
  if (it == resource_to_redirect_chain_map_.end()) {
    return NULL;
  } else {
    return it->second;
  }
}

const Resource* RedirectRegistry::GetFinalRedirectTarget(
    const Resource* resource) const {
  // If resource is NULL, GetRedirectChainOrNull will return NULL, and so we'll
  // return resource, which is NULL, which is what we want.
  const RedirectRegistry::RedirectChain* chain =
      GetRedirectChainOrNull(resource);
  return chain ? chain->back() : resource;
}

}  // namespace pagespeed
