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

#include "pagespeed/core/resource_evaluation.h"

#include <algorithm>
#include <iterator>
#include <map>
#include <string>

#include "base/logging.h"
#include "base/stl_util-inl.h"
#include "pagespeed/core/browsing_context.h"
#include "pagespeed/core/resource_fetch.h"

namespace {

}  // namespace

namespace pagespeed {

ResourceEvaluation::ResourceEvaluation(const std::string& uri,
                                       const BrowsingContext* context,
                                       const Resource* resource,
                                       const PagespeedInput* pagespeed_input)
    : pagespeed_input_(pagespeed_input),
      resource_(resource),
      context_(context),
      finalized_(false),
      data_(new ResourceEvaluationData()) {
  data_->set_uri(uri);
  data_->set_resource_url(resource->GetRequestUrl());
}

ResourceEvaluation::~ResourceEvaluation() {
  STLDeleteContainerPointers(constraints_.begin(), constraints_.end());
}

ResourceEvaluationConstraint* ResourceEvaluation::AddConstraint() {
  if (finalized_) {
    LOG(DFATAL) << "Attempting to modify finalized ResourceEvaluation "
                << GetResourceEvaluationUri();
  }
  ResourceEvaluationConstraint* result = new ResourceEvaluationConstraint(
      pagespeed_input_);
  constraints_.push_back(result);
  return result;
}

bool ResourceEvaluation::SetFetch(const ResourceFetch& fetch) {
  if (finalized_) {
    LOG(DFATAL) << "Attempting to modify finalized ResourceEvaluation "
                << GetResourceEvaluationUri();
  }
  data_->set_fetch_uri(fetch.GetResourceFetchUri());
  return true;
}

void ResourceEvaluation::SetTiming(int64 start_tick, int64 start_time_msec,
                                   int64 finish_tick, int64 finish_time_msec) {
  if (finalized_) {
    LOG(DFATAL) << "Attempting to modify finalized ResourceEvaluation "
                << GetResourceEvaluationUri();
  }
  data_->mutable_start()->set_tick(start_tick);
  data_->mutable_start()->set_msec(start_time_msec);

  data_->mutable_finish()->set_tick(finish_tick);
  data_->mutable_finish()->set_msec(finish_time_msec);
}

void ResourceEvaluation::SetIsMatchingMediaType(bool is_matching_media_type) {
  if (finalized_) {
    LOG(DFATAL) << "Attempting to modify finalized ResourceEvaluation "
                << GetResourceEvaluationUri();
  }
  data_->set_is_matching_media_type(is_matching_media_type);
}

void ResourceEvaluation::SetIsAsync(bool is_async) {
  if (finalized_) {
    LOG(DFATAL) << "Attempting to modify finalized ResourceEvaluation "
                << GetResourceEvaluationUri();
  }
  data_->set_is_async(is_async);
}

void ResourceEvaluation::SetIsDefer(bool is_defer) {
  if (finalized_) {
    LOG(DFATAL) << "Attempting to modify finalized ResourceEvaluation "
                << GetResourceEvaluationUri();
  }
  data_->set_is_defer(is_defer);
}

void ResourceEvaluation::SetEvaluationLines(int32 start_line, int32 end_line) {
  if (finalized_) {
    LOG(DFATAL) << "Attempting to modify finalized ResourceEvaluation "
                << GetResourceEvaluationUri();
  }
  data_->set_block_start_line(start_line);
  data_->set_block_end_line(end_line);
}

void ResourceEvaluation::SetEvaluationType(EvaluationType type) {
  if (finalized_) {
    LOG(DFATAL) << "Attempting to modify finalized ResourceEvaluation "
                << GetResourceEvaluationUri();
  }
  data_->set_type(type);
}

bool ResourceEvaluation::Finalize() {
  if (finalized_) {
    LOG(DFATAL) << "Attempting to finalized ResourceEvaluation twice "
                << GetResourceEvaluationUri();
  }

  finalized_ = true;
  return true;
}

ResourceType ResourceEvaluation::GetResourceType() const {
  return GetResource().GetResourceType();
}

const ResourceFetch* ResourceEvaluation::GetFetch() const {
  if (!data_->has_fetch_uri()) {
    return NULL;
  }

  const TopLevelBrowsingContext* context = pagespeed_input_
      ->GetTopLevelBrowsingContext();
  return context->FindResourceFetch(data_->fetch_uri());
}

bool ResourceEvaluation::GetConstraints(
    EvaluationConstraintVector* constraints) const {
  DCHECK(constraints->empty());
  constraints->assign(constraints_.begin(), constraints_.end());
  return !constraints->empty();
}
int32 ResourceEvaluation::GetConstraintCount() const {
  return constraints_.size();
}

const ResourceEvaluationConstraint& ResourceEvaluation::GetConstraint(
    int index) const {
  if (index < 0 || static_cast<size_t>(index) >= constraints_.size()) {
    LOG(DFATAL) << "Index out of bounds.";
  }
  return *constraints_[index];
}

ResourceEvaluationConstraint* ResourceEvaluation::GetMutableConstraint(
    int index) {
  if (finalized_) {
    LOG(DFATAL) << "Attempting to modify finalized ResourceEvaluation "
                << GetResourceEvaluationUri();
  }
  return const_cast<ResourceEvaluationConstraint*>(&GetConstraint(index));
}

bool ResourceEvaluation::SerializeData(ResourceEvaluationData* data) const {
  data->CopyFrom(*data_);
  DCHECK_EQ(0, data_->constraints_size());

  for (std::vector<ResourceEvaluationConstraint*>::const_iterator it =
      constraints_.begin(); it != constraints_.end(); ++it) {
    (*it)->SerializeData(data->add_constraints());
  }
  return true;
}

ResourceEvaluationConstraint::ResourceEvaluationConstraint(
    const PagespeedInput* pagespeed_input)
    : pagespeed_input_(pagespeed_input),
      data_(new ResourceEvaluationConstraintData()) {
}

ResourceEvaluationConstraint::~ResourceEvaluationConstraint() {
}

bool ResourceEvaluationConstraint::SetPredecessor(
    const ResourceEvaluation* predecessor) {
  data_->set_predecessor_uri(predecessor->GetResourceEvaluationUri());
  return true;
}

const ResourceEvaluation* ResourceEvaluationConstraint::GetPredecessor() const {
  if (!data_->has_predecessor_uri()) {
    return NULL;
  }

  const TopLevelBrowsingContext* context = pagespeed_input_
      ->GetTopLevelBrowsingContext();
  return context->FindResourceEvaluation(data_->predecessor_uri());
}

bool ResourceEvaluationConstraint::SerializeData(
    ResourceEvaluationConstraintData* data) const {
  data->CopyFrom(*data_);
  return true;
}

}  // namespace pagespeed
