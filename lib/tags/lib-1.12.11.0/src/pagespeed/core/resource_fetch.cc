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

#include "pagespeed/core/resource_fetch.h"

#include <map>
#include <set>

#include "base/logging.h"
#include "base/stl_util-inl.h"
#include "pagespeed/core/browsing_context.h"
#include "pagespeed/core/resource_evaluation.h"

namespace {

}  // namespace

namespace pagespeed {

ResourceFetch::ResourceFetch(const std::string& uri,
                             const TopLevelBrowsingContext* context,
                             const Resource* resource)
    : resource_(resource),
      context_(context),
      finalized_(false),
      logical_download_(new ResourceFetchDownload(context)),
      data_(new ResourceFetchData()) {
  data_->set_uri(uri);
  data_->set_resource_url(resource->GetRequestUrl());
}

ResourceFetch::~ResourceFetch() {
  STLDeleteContainerPointers(delays_.begin(), delays_.end());
}


ResourceFetchDelay* ResourceFetch::AddFetchDelay() {
  if (finalized_) {
    LOG(DFATAL) << "Attempting to modify finalized ResourceFetch "
                << GetResourceFetchUri();
  }
  ResourceFetchDelay* result = new ResourceFetchDelay();
  delays_.push_back(result);
  return result;
}

ResourceFetchDownload* ResourceFetch::GetMutableDownload() {
  if (finalized_) {
    LOG(DFATAL) << "Attempting to modify finalized ResourceFetch "
                << GetResourceFetchUri();
  }
  return logical_download_.get();
}

void ResourceFetch::SetDiscoveryType(ResourceDiscoveryType discovery_type) {
  if (finalized_) {
    LOG(DFATAL) << "Attempting to modify finalized ResourceFetch "
                << GetResourceFetchUri();
  }
  data_->set_type(discovery_type);
}

bool ResourceFetch::AcquireCodeLocation(
    std::vector<CodeLocation*>* location_stack) {
  if (finalized_) {
    LOG(DFATAL) << "Attempting to modify finalized ResourceFetch "
                << GetResourceFetchUri();
  }
  data_->clear_location();
  for (std::vector<CodeLocation*>::const_iterator it = location_stack->begin();
      it != location_stack->end(); ++it) {
    CodeLocation* location = data_->add_location();
    location->CopyFrom(**it);
    delete *it;
  }
  location_stack->clear();
  return true;
}

bool ResourceFetch::Finalize() {
  if (finalized_) {
    LOG(DFATAL) << "Attempting to finalize ResourceFetch twice "
                << GetResourceFetchUri();
  }

  if (data_->type() == UNKNOWN_DISCOVERY_TYPE) {
    // The discovery type has not yet been specified
    // Find the head ResourceFetch in this redirect chain.
    std::set<const ResourceFetch*> visited;
    const ResourceFetch* redirect_head = this;
    do {
      if (visited.find(redirect_head) != visited.end()) {
        LOG(INFO) << "Encountered redirect loop.";
        break;
      }
      visited.insert(redirect_head);

      const ResourceEvaluation* requestor = redirect_head->GetRequestor();
      if (requestor == NULL) {
        break;
      }
      const ResourceFetch* previous_fetch = requestor->GetFetch();
      if (previous_fetch == NULL ||
          previous_fetch->GetResource().GetResourceType() != REDIRECT) {
        break;
      }
      redirect_head = previous_fetch;
    } while (true);

    if (redirect_head != NULL && redirect_head != this) {
      redirect_download_.reset(new ResourceFetchDownload(context_));
      if (!redirect_download_->CopyFrom(*logical_download_, false)) {
        return false;
      }

      if (!logical_download_->CopyFrom(*redirect_head->logical_download_,
                                       true)) {
        return false;
      }

      if (redirect_head->data_->has_type()) {
        data_->set_type(redirect_head->data_->type());
      } else {
        data_->clear_type();
      }

      data_->clear_location();
      for (int i = 0; i < redirect_head->data_->location_size(); i++) {
        data_->add_location()->CopyFrom(redirect_head->data_->location(i));
      }

      for (int i = 0; i < redirect_head->GetFetchDelayCount(); i++) {
        ResourceFetchDelay* delay = AddFetchDelay();
        if (!delay->CopyFrom(redirect_head->GetFetchDelay(i))) {
          return false;
        }
      }
    }
  }

  finalized_ = true;
  return true;
}

const ResourceEvaluation* ResourceFetch::GetRequestor() const {
  return logical_download_->GetRequestor();
}

int64 ResourceFetch::GetStartTick() const {
  return logical_download_->GetStartTick();
}

int64 ResourceFetch::GetFinishTick() const {
  return logical_download_->GetFinishTick();
}

bool ResourceFetch::GetCodeLocation(
    std::vector<const CodeLocation*>* location) const {
  location->clear();
  for (int i = 0; i < data_->location_size(); ++i) {
    location->push_back(&data_->location(i));
  }
  return !location->empty();
}

int ResourceFetch::GetCodeLocationCount() const {
  return data_->location_size();
}

const CodeLocation& ResourceFetch::GetCodeLocation(int index) const {
  if (index < 0 || index >= data_->location_size()) {
    LOG(DFATAL)  << "Index out of bounds.";
  }
  return data_->location(index);
}

int ResourceFetch::GetFetchDelayCount() const {
  return delays_.size();
}

const ResourceFetchDelay& ResourceFetch::GetFetchDelay(int index) const {
  if (index < 0 || static_cast<size_t>(index) >= delays_.size()) {
    LOG(DFATAL)  << "Index out of bounds.";
  }
  return *delays_.at(index);
}

bool ResourceFetch::SerializeData(ResourceFetchData* data) const {
  data->CopyFrom(*data_);
  for (std::vector<ResourceFetchDelay*>::const_iterator it = delays_.begin();
      it != delays_.end(); ++it) {
    if (!(*it)->SerializeData(data->add_delay())) {
      return false;
    }
  }

  logical_download_->SerializeData(data->mutable_download());
  if (redirect_download_ != NULL) {
    redirect_download_->SerializeData(data->mutable_redirect_download());
  }

  return true;
}

ResourceFetchDownload::ResourceFetchDownload(
    const TopLevelBrowsingContext* context)
    : context_(context),
      data_(new ResourceFetchDownloadData()) {
}
ResourceFetchDownload::~ResourceFetchDownload() {
}

bool ResourceFetchDownload::SetRequestor(const ResourceEvaluation* requestor) {
  if (requestor != NULL) {
    data_->set_requestor_uri(requestor->GetResourceEvaluationUri());
  } else {
    data_->clear_requestor_uri();
  }
  return true;
}

void ResourceFetchDownload::SetLoadTiming(int64 start_tick,
                                          int64 start_time_msec,
                                          int64 finish_tick,
                                          int64 finish_time_msec) {
  data_->mutable_start()->set_tick(start_tick);
  data_->mutable_start()->set_msec(start_time_msec);

  data_->mutable_finish()->set_tick(finish_tick);
  data_->mutable_finish()->set_msec(finish_time_msec);
}

bool ResourceFetchDownload::CopyFrom(const ResourceFetchDownload& download,
                                     bool keep_finish_time) {
  if (keep_finish_time &&
      data_->finish().tick() < download.data_->start().tick()) {
    return false;
  }

  data_->set_requestor_uri(download.data_->requestor_uri());
  data_->mutable_start()->CopyFrom(download.data_->start());
  if (!keep_finish_time) {
    data_->mutable_finish()->CopyFrom(download.data_->finish());
  }
  return true;
}

const ResourceEvaluation* ResourceFetchDownload::GetRequestor() const {
  if (!data_->has_requestor_uri()) {
    return NULL;
  }

  return context_->FindResourceEvaluation(data_->requestor_uri());
}

bool ResourceFetchDownload::SerializeData(
    ResourceFetchDownloadData* data) const {
  data->CopyFrom(*data_);
  return true;
}

ResourceFetchDelay::ResourceFetchDelay()
    : data_(new ResourceFetchDelayData()) {
}

ResourceFetchDelay::~ResourceFetchDelay() {
}

bool ResourceFetchDelay::CopyFrom(const ResourceFetchDelay& delay) {
  data_->CopyFrom(*delay.data_);
  return true;
}

bool ResourceFetchDelay::AcquireCodeLocation(
    std::vector<CodeLocation*>* location_stack) {
  data_->clear_location();
  for (std::vector<CodeLocation*>::const_iterator it = location_stack->begin();
      it != location_stack->end(); ++it) {
    CodeLocation* location = data_->add_location();
    location->CopyFrom(**it);
    delete *it;
  }
  location_stack->clear();
  return true;
}

bool ResourceFetchDelay::GetCodeLocation(
    std::vector<const CodeLocation*>* location) const {
  location->clear();
  for (int i = 0; i < data_->location_size(); ++i) {
    location->push_back(&data_->location(i));
  }
  return !location->empty();
}

int ResourceFetchDelay::GetCodeLocationCount() const {
  return data_->location_size();
}

const CodeLocation& ResourceFetchDelay::GetCodeLocation(int index) const {
  if (index < 0 || index >= data_->location_size()) {
    LOG(DFATAL)  << "Index out of bounds.";
  }
  return data_->location(index);
}

bool ResourceFetchDelay::SerializeData(ResourceFetchDelayData* data) const {
  data->CopyFrom(*data_);
  return true;
}

}  // namespace pagespeed
