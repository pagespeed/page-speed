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

#include <map>
#include <set>
#include <string>
#include <vector>

#include "base/logging.h"
#include "googleurl/src/gurl.h"
#include "googleurl/src/url_canon.h"
#include "pagespeed/core/formatter.h"
#include "pagespeed/core/pagespeed_input.h"
#include "pagespeed/core/resource.h"
#include "pagespeed/core/resource_util.h"
#include "pagespeed/core/result_provider.h"
#include "pagespeed/core/rule_input.h"
#include "pagespeed/core/uri_util.h"
#include "pagespeed/l10n/l10n.h"
#include "pagespeed/proto/pagespeed_output.pb.h"

namespace {

class RedirectGraph {
 public:
  explicit RedirectGraph(const pagespeed::PagespeedInput* pagespeed_input)
      : pagespeed_input_(pagespeed_input) {}
  void AddResource(const pagespeed::Resource& resource);
  void AppendRedirectChainResults(
      pagespeed::RuleInput::RedirectChainVector* chains);

 private:
  // Build a prioritized vector of possible roots.
  // This vector should contain all redirect sources, but give
  // priority to those that are not redirect targets.  We cannot
  // exclude all redirect targets because we would like to warn about
  // pure redirect loops.
  void GetPriorizedRoots(std::vector<std::string>* roots);
  void PopulateRedirectChainResult(const std::string& root,
                                   pagespeed::RuleInput::RedirectChain* chain);

  typedef std::map<std::string, std::vector<std::string> > RedirectMap;
  RedirectMap redirect_map_;
  std::set<std::string> destinations_;
  std::set<std::string> processed_;
  const pagespeed::PagespeedInput* pagespeed_input_;
};

void RedirectGraph::AddResource(const pagespeed::Resource& resource) {
  std::string destination =
      pagespeed::resource_util::GetRedirectedUrl(resource);
  if (!destination.empty()) {
    redirect_map_[resource.GetRequestUrl()].push_back(destination);
    destinations_.insert(destination);
  }
}

void RedirectGraph::AppendRedirectChainResults(
    pagespeed::RuleInput::RedirectChainVector* chains) {
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
    pagespeed::RuleInput::RedirectChain chain;
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
    const std::string& root, pagespeed::RuleInput::RedirectChain* chain) {
  // Perform a DFS on the redirect graph.
  std::vector<std::string> work_stack;
  work_stack.push_back(root);
  while (!work_stack.empty()) {
    std::string current = work_stack.back();
    work_stack.pop_back();
    const pagespeed::Resource* resource =
        pagespeed_input_->GetResourceWithUrlOrNull(current);
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

namespace pagespeed {

RuleInput::RuleInput(const PagespeedInput& pagespeed_input)
    : pagespeed_input_(&pagespeed_input),
      initialized_(false) {
  if (!pagespeed_input_->is_frozen()) {
    LOG(DFATAL) << "Passed non-frozen PagespeedInput to RuleInput.";
  }
}

void RuleInput::BuildRedirectChains() {
  RedirectGraph redirect_graph(pagespeed_input_);
  for (int idx = 0, num = pagespeed_input_->num_resources(); idx < num; ++idx) {
    redirect_graph.AddResource(pagespeed_input_->GetResource(idx));
  }

  redirect_chains_.clear();
  redirect_graph.AppendRedirectChainResults(&redirect_chains_);

  // Map resource to chains.
  for (RuleInput::RedirectChainVector::const_iterator it =
       redirect_chains_.begin(), end = redirect_chains_.end();
       it != end;
       ++it) {
    const RuleInput::RedirectChain& chain = *it;
    for (RuleInput::RedirectChain::const_iterator cit = chain.begin(),
        cend = chain.end();
        cit != cend;
        ++cit) {
      resource_to_redirect_chain_map_[*cit] = &chain;
    }
  }
}

void RuleInput::Init() {
  if (!initialized_) {
    BuildRedirectChains();
    initialized_ = true;
  }
}

const RuleInput::RedirectChainVector& RuleInput::GetRedirectChains() const {
  DCHECK(initialized_);
  return redirect_chains_;
};

const RuleInput::RedirectChain* RuleInput::GetRedirectChainOrNull(
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

const Resource* RuleInput::GetFinalRedirectTarget(
    const Resource* resource) const {
  // If resource is NULL, GetRedirectChainOrNull will return NULL, and so we'll
  // return resource, which is NULL, which is what we want.
  const RuleInput::RedirectChain* chain = GetRedirectChainOrNull(resource);
  return chain ? chain->back() : resource;
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
  if (::pagespeed::resource_util::IsCompressibleResource(resource)) {
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
