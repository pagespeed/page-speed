// Copyright 2011 Google Inc. All Rights Reserved.
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

#include "pagespeed/core/browsing_context.h"

#include <algorithm>
#include <iterator>
#include <map>
#include <string>

#include "base/logging.h"
#include "base/stl_util-inl.h"
#include "pagespeed/core/dom.h"
#include "pagespeed/core/resource.h"
#include "pagespeed/core/resource_evaluation.h"
#include "pagespeed/core/resource_fetch.h"
#include "pagespeed/core/resource_util.h"
#include "pagespeed/core/uri_util.h"

namespace {

template<class ConstraintMap>
void DeleteResourceDataPointers(ConstraintMap* constraint_map) {
  for (typename ConstraintMap::iterator it = constraint_map->begin();
      it != constraint_map->end(); ++it) {
    STLDeleteContainerPointers(it->second.begin(), it->second.end());
  }
}

}  // namespace

namespace pagespeed {

// This utility class generates URIs for BrowsingContext, ResourceFetches and
// ResourceEvaluations while incrementing the sequence number for [type, url]
// pairs.
class ActionUriGenerator {
 public:
  bool GenerateUniqueUri(uri_util::UriType type, const std::string& url,
                         std::string* action_uri) {
    int sequence_number = 1;
    std::map<std::string, int>::const_iterator it = sequence_[type].find(url);
    if (it != sequence_[type].end()) {
      sequence_number = it->second;
    }
    sequence_[type][url] = sequence_number + 1;

    return uri_util::GetActionUriFromResourceUrl(type, url, sequence_number,
                                                 action_uri);
  }

 private:
  std::map<uri_util::UriType, std::map<std::string, int> > sequence_;
};

BrowsingContext::BrowsingContext(
    const Resource* document_resource, const BrowsingContext* parent_context,
    TopLevelBrowsingContext* top_level_context,
    ActionUriGenerator* action_uri_generator,
    const PagespeedInput* pagespeed_input)
    : pagespeed_input_(pagespeed_input),
      action_uri_generator_(action_uri_generator),
      finalized_(false),
      top_level_context_(top_level_context),
      parent_context_(parent_context),
      document_resource_(document_resource),
      document_(NULL),
      event_dom_content_msec_(-1),
      event_dom_content_tick_(-1),
      event_load_msec_(-1),
      event_load_tick_(-1) {
  if (document_resource != NULL) {
    RegisterResource(document_resource);
  }

  // Walk up the browsing context chain to find the next context with a document
  // resource associated. This applies ie for frames that were created by
  // JavaScript but without supplying a resource url.
  // We use the resource URL found to generate the URI of this browsing context.
  const BrowsingContext* document_context = this;
  while (document_context != NULL &&
      document_context->GetDocumentResourceOrNull() == NULL) {
    document_context = document_context->GetParentContext();
  }

  std::string context_document_url;
  if (document_context != NULL &&
      document_context->GetDocumentResourceOrNull() != NULL) {
    context_document_url =
        document_context->GetDocumentResourceOrNull()->GetRequestUrl();

    action_uri_generator_->GenerateUniqueUri(uri_util::BROWSING_CONTEXT,
                                             context_document_url, &uri_);
  } else {
    // The public constructor for top-level contexts checks for a non-NULL
    // document resource, thus we must always find a non-NULL document resource
    // in the parent context chain.
    LOG(DFATAL) << "No parent browsing context with associated resource set.";
  }
}

BrowsingContext::~BrowsingContext() {
  STLDeleteContainerPointers(nested_contexts_.begin(), nested_contexts_.end());

  DeleteResourceDataPointers(&resource_fetch_map_);
  DeleteResourceDataPointers(&resource_evaluation_map_);
}

BrowsingContext* BrowsingContext::AddNestedBrowsingContext(
    const Resource* resource) {
  if (finalized_) {
    LOG(DFATAL) << "Attempting to modify finalized BrowsingContext "
                << GetBrowsingContextUri();
  }

  BrowsingContext* nested_context = new BrowsingContext(resource, this,
                                                        top_level_context_,
                                                        action_uri_generator_,
                                                        pagespeed_input_);
  nested_contexts_.push_back(nested_context);
  RegisterBrowsingContext(nested_context);
  return nested_context;
}

ResourceFetch* BrowsingContext::AddResourceFetch(const Resource* resource) {
  if (finalized_) {
    LOG(DFATAL) << "Attempting to modify finalized BrowsingContext "
                << GetBrowsingContextUri();
  }

  if (!RegisterResource(resource)) {
    return NULL;
  }
  std::string fetch_uri;
  action_uri_generator_->GenerateUniqueUri(uri_util::FETCH,
                                           resource->GetRequestUrl(),
                                           &fetch_uri);

  ResourceFetch* result = new ResourceFetch(fetch_uri, this, resource,
                                            pagespeed_input_);
  resource_fetch_map_[resource].push_back(result);
  RegisterResourceFetch(result);
  return result;
}

ResourceEvaluation* BrowsingContext::AddResourceEvaluation(
    const Resource* resource) {
  if (finalized_) {
    LOG(DFATAL) << "Attempting to modify finalized BrowsingContext "
                << GetBrowsingContextUri();
  }

  if (!RegisterResource(resource)) {
    return NULL;
  }
  std::string eval_uri;
  action_uri_generator_->GenerateUniqueUri(uri_util::EVAL,
                                           resource->GetRequestUrl(),
                                           &eval_uri);

  ResourceEvaluation* result = new ResourceEvaluation(eval_uri, this, resource,
                                                      pagespeed_input_);
  resource_evaluation_map_[resource].push_back(result);
  RegisterResourceEvaluation(result);
  return result;
}

void BrowsingContext::SetEventDomContentTiming(int64 tick, int64 time_msec) {
  if (finalized_) {
    LOG(DFATAL) << "Attempting to modify finalized BrowsingContext "
                << GetBrowsingContextUri();
  }

  event_dom_content_tick_ = tick;
  event_dom_content_msec_ = time_msec;
}

void BrowsingContext::SetEventLoadTiming(int64 tick, int64 time_msec) {
  if (finalized_) {
    LOG(DFATAL) << "Attempting to modify finalized BrowsingContext "
                << GetBrowsingContextUri();
  }

  event_load_tick_ = tick;
  event_load_msec_ = time_msec;
}

void BrowsingContext::AcquireDomDocument(DomDocument* document) {
  if (finalized_) {
    LOG(DFATAL) << "Attempting to modify finalized BrowsingContext "
                << GetBrowsingContextUri();
  }

  document_.reset(document);
}

bool BrowsingContext::Finalize() {
  if (finalized_) {
    LOG(DFATAL) << "Attempting to finalize BrowsingContext twice "
                << GetBrowsingContextUri();
  }

  // Ensure that all ResourceFetches and ResourceEvals are finalized at this
  // point.
  for (ResourceFetchMap::const_iterator map_it = resource_fetch_map_.begin();
      map_it != resource_fetch_map_.end(); ++map_it) {
    for (std::vector<ResourceFetch*>::const_iterator it =
        map_it->second.begin(); it != map_it->second.end(); ++it) {
      if (!(*it)->IsFinalized() && !(*it)->Finalize()) {
        return false;
      }
    }
  }

  for (ResourceEvalMap::const_iterator map_it =
      resource_evaluation_map_.begin();
      map_it != resource_evaluation_map_.end(); ++map_it) {
    for (std::vector<ResourceEvaluation*>::const_iterator it =
        map_it->second.begin(); it != map_it->second.end(); ++it) {
      if (!(*it)->IsFinalized() && !(*it)->Finalize()) {
        return false;
      }
    }
  }

  for (std::vector<BrowsingContext*>::const_iterator it =
      nested_contexts_.begin();
      it != nested_contexts_.end(); ++it) {
    if (!(*it)->Finalize()) {
      return false;
    }
  }

  finalized_ = true;
  return true;
}

const Resource* BrowsingContext::GetDocumentResourceOrNull() const {
  return document_resource_;
}

const std::string& BrowsingContext::GetBrowsingContextUri() const {
  return uri_;
}

const DomDocument* BrowsingContext::GetDomDocument() const {
  return document_.get();
}

const BrowsingContext* BrowsingContext::GetParentContext() const {
  return parent_context_;
}

bool BrowsingContext::GetNestedContexts(BrowsingContextVector* contexts) const {
  DCHECK(contexts->empty());
  contexts->assign(nested_contexts_.begin(), nested_contexts_.end());
  return !contexts->empty();
}

int32 BrowsingContext::GetNestedContextCount() const {
  return nested_contexts_.size();
}

const BrowsingContext& BrowsingContext::GetNestedContext(int index) const {
  if (index < 0 || static_cast<size_t>(index) >= nested_contexts_.size()) {
    LOG(DFATAL) << "Index out of bounds.";
  }
  return *nested_contexts_[index];
}

BrowsingContext* BrowsingContext::GetMutableNestedContext(int index) {
  if (finalized_) {
    LOG(DFATAL) << "Attempting to modify finalized BrowsingContext "
                << GetBrowsingContextUri();
  }

  return const_cast<BrowsingContext*>(&GetNestedContext(index));
}

bool BrowsingContext::GetResources(ResourceVector* resources) const {
  DCHECK(resources->empty());
  resources->assign(resources_.begin(), resources_.end());
  return !resources->empty();
}

bool BrowsingContext::GetResourceFetches(const Resource& resource,
                                         ResourceFetchVector* requests) const {
  DCHECK(requests->empty());
  ResourceFetchMap::const_iterator it = resource_fetch_map_.find(&resource);
  if (it == resource_fetch_map_.end()) {
    return false;
  }

  requests->assign(it->second.begin(), it->second.end());
  return !requests->empty();
}

int32 BrowsingContext::GetResourceFetchCount(const Resource& resource) const {
  ResourceFetchMap::const_iterator it = resource_fetch_map_.find(&resource);
  if (it == resource_fetch_map_.end()) {
    return 0;
  }

  return it->second.size();
}

const ResourceFetch& BrowsingContext::GetResourceFetch(const Resource& resource,
                                                       int index) const {
  ResourceFetchMap::const_iterator it = resource_fetch_map_.find(&resource);
  if (it == resource_fetch_map_.end()) {
    LOG(DFATAL) << "Requested ResourceFetch for unknown resource";
  }

  if (index < 0 || static_cast<size_t>(index) >= it->second.size()) {
    LOG(DFATAL) << "Index out of bounds.";
  }

  return *it->second[index];
}

ResourceFetch* BrowsingContext::GetMutableResourceFetch(
    const Resource& resource, int index) {
  if (finalized_) {
    LOG(DFATAL) << "Attempting to modify finalized BrowsingContext "
                << GetBrowsingContextUri();
  }

  return const_cast<ResourceFetch*>(&GetResourceFetch(resource, index));
}

bool BrowsingContext::GetResourceEvaluations(
    const Resource& resource, ResourceEvaluationVector* evaluations) const {
  ResourceEvalMap::const_iterator it = resource_evaluation_map_.find(&resource);
  DCHECK(evaluations->empty());
  if (it == resource_evaluation_map_.end()) {
    return false;
  }

  evaluations->assign(it->second.begin(), it->second.end());
  return !evaluations->empty();
}

int32 BrowsingContext::GetResourceEvaluationCount(
    const Resource& resource) const {
  ResourceEvalMap::const_iterator it = resource_evaluation_map_.find(&resource);
  if (it == resource_evaluation_map_.end()) {
    return 0;
  }

  return it->second.size();
}

const ResourceEvaluation& BrowsingContext::GetResourceEvaluation(
    const Resource& resource, int index) const {
  ResourceEvalMap::const_iterator it = resource_evaluation_map_.find(&resource);
  if (it == resource_evaluation_map_.end()) {
    LOG(DFATAL) << "Requested ResourceFetch for unknown resource";
  }

  if (index < 0 || static_cast<size_t>(index) >= it->second.size()) {
    LOG(DFATAL) << "Index out of bounds.";
  }

  return *it->second[index];
}

ResourceEvaluation* BrowsingContext::GetMutableResourceEvaluation(
    const Resource& resource, int index) {
  if (finalized_) {
    LOG(DFATAL) << "Attempting to modify finalized BrowsingContext "
                << GetBrowsingContextUri();
  }

  return const_cast<ResourceEvaluation*>(&GetResourceEvaluation(resource,
                                                                index));
}

bool BrowsingContext::RegisterBrowsingContext(const BrowsingContext* context) {
  DCHECK(this != top_level_context_);
  return implicit_cast<BrowsingContext*>(top_level_context_)
      ->RegisterBrowsingContext(context);
}

bool BrowsingContext::RegisterResourceFetch(const ResourceFetch* fetch) {
  DCHECK(this != top_level_context_);
  return implicit_cast<BrowsingContext*>(top_level_context_)
      ->RegisterResourceFetch(fetch);
}

bool BrowsingContext::RegisterResourceEvaluation(
    const ResourceEvaluation* eval) {
  DCHECK(this != top_level_context_);
  return implicit_cast<BrowsingContext*>(top_level_context_)
      ->RegisterResourceEvaluation(eval);
}

bool BrowsingContext::RegisterResource(const Resource* child_resource) {
  if (finalized_) {
    LOG(DFATAL) << "Attempting to modify finalized BrowsingContext "
                << GetBrowsingContextUri();
  }

  if (pagespeed_input_->GetResourceWithUrlOrNull(
      child_resource->GetRequestUrl()) != child_resource) {
    LOG(DFATAL) << "Cannot register child resource which is not added to the "
                "PagespeedInput.";
    return false;
  }

  resources_.insert(child_resource);

  std::set<const Resource*> visited;
  const Resource* candidate_resource = child_resource;
  while (candidate_resource->GetResourceType() == REDIRECT) {
    if (visited.find(candidate_resource) != visited.end()) {
      LOG(INFO) << "Encountered redirect loop.";
      break;
    }
    visited.insert(candidate_resource);

    std::string target_url = resource_util::GetRedirectedUrl(
        *candidate_resource);
    candidate_resource = pagespeed_input_->GetResourceWithUrlOrNull(target_url);
    if (candidate_resource == NULL) {
      break;
    }
    resources_.insert(candidate_resource);
  }

  return true;
}

bool BrowsingContext::SerializeData(BrowsingContextData* data) const {
  data->set_uri(uri_);
  if (document_resource_ != NULL) {
    data->set_document_resource_url(document_resource_->GetRequestUrl());
  }

  for (ResourceSet::const_iterator it = resources_.begin();
      it != resources_.end(); ++it) {
    const Resource& resource = **it;
    data->add_resource_urls(resource.GetRequestUrl());

    ResourceFetchVector fetches;
    if (GetResourceFetches(resource, &fetches)) {
      for (ResourceFetchVector::const_iterator fetch_it = fetches.begin();
          fetch_it != fetches.end(); ++fetch_it) {
        (*fetch_it)->SerializeData(data->add_fetch());
      }
    }

    ResourceEvaluationVector evals;
    if (GetResourceEvaluations(resource, &evals)) {
      for (ResourceEvaluationVector::const_iterator eval_it = evals.begin();
          eval_it != evals.end(); ++eval_it) {
        (*eval_it)->SerializeData(data->add_evaluation());
      }
    }
  }

  for (std::vector<BrowsingContext*>::const_iterator it =
      nested_contexts_.begin(); it != nested_contexts_.end(); ++it) {
    (*it)->SerializeData(data->add_nested_context());
  }

  if (event_dom_content_msec_ != -1 || event_dom_content_tick_ != -1) {
    Timestamp* timestamp = data->mutable_event_dom_content();
    if (event_dom_content_msec_ != -1) {
      timestamp->set_msec(event_dom_content_msec_);
    }
    if (event_dom_content_tick_ != -1) {
      timestamp->set_tick(event_dom_content_tick_);
    }
  }

  if (event_load_msec_ != -1 || event_load_tick_ != -1) {
    Timestamp* timestamp = data->mutable_event_on_load();
    if (event_load_msec_ != -1) {
      timestamp->set_msec(event_load_msec_);
    }
    if (event_load_tick_ != -1) {
      timestamp->set_tick(event_load_tick_);
    }
  }

  return true;
}

TopLevelBrowsingContext::TopLevelBrowsingContext(
    const Resource* document_resource, const PagespeedInput* pagespeed_input)
    : BrowsingContext(document_resource, NULL, this, new ActionUriGenerator(),
                      pagespeed_input) {
  RegisterBrowsingContext(this);
}

TopLevelBrowsingContext::~TopLevelBrowsingContext() {
  delete action_uri_generator();
}

const BrowsingContext* TopLevelBrowsingContext::FindBrowsingContext(
    const std::string& context_uri) const {
  std::map<std::string, const BrowsingContext*>::const_iterator it =
      uri_browsing_context_map_.find(context_uri);

  return it != uri_browsing_context_map_.end() ? it->second : NULL;
}

const ResourceFetch* TopLevelBrowsingContext::FindResourceFetch(
    const std::string& fetch_uri) const {
  std::map<std::string, const ResourceFetch*>::const_iterator it =
      uri_resource_fetch_map_.find(fetch_uri);

  return it != uri_resource_fetch_map_.end() ? it->second : NULL;
}

const ResourceEvaluation* TopLevelBrowsingContext::FindResourceEvaluation(
    const std::string& eval_uri) const {
  std::map<std::string, const ResourceEvaluation*>::const_iterator it =
      uri_resource_eval_map_.find(eval_uri);

  return it != uri_resource_eval_map_.end() ? it->second : NULL;
}

bool TopLevelBrowsingContext::RegisterBrowsingContext(
    const BrowsingContext* context) {
  uri_browsing_context_map_[context->GetBrowsingContextUri()] = context;
  return true;
}

bool TopLevelBrowsingContext::RegisterResourceFetch(
    const ResourceFetch* fetch) {
  uri_resource_fetch_map_[fetch->GetResourceFetchUri()] = fetch;
  return true;
}

bool TopLevelBrowsingContext::RegisterResourceEvaluation(
    const ResourceEvaluation* eval) {
  uri_resource_eval_map_[eval->GetResourceEvaluationUri()] = eval;
  return true;
}

}  // namespace pagespeed
