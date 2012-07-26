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

#include <algorithm>
#include <map>

#include "base/logging.h"
#include "base/stl_util-inl.h"
#include "pagespeed/core/browsing_context.h"
#include "pagespeed/core/dom.h"
#include "pagespeed/core/image_attributes.h"
#include "pagespeed/core/resource.h"
#include "pagespeed/core/resource_util.h"
#include "pagespeed/core/string_util.h"
#include "pagespeed/core/uri_util.h"
#include "pagespeed/proto/pagespeed_output.pb.h"
#include "pagespeed/proto/resource.pb.h"
#include "pagespeed/proto/timeline.pb.h"

using pagespeed::string_util::LowerCaseEqualsASCII;

namespace pagespeed {

namespace {

// sorts resources by their request start times.
struct ResourceRequestStartTimeLessThan {
  bool operator() (const Resource* lhs, const Resource* rhs) const {
    return lhs->IsRequestStartTimeLessThan(*rhs);
  }
};

}  // namespace

PagespeedInput::PagespeedInput()
    : input_info_(new InputInformation),
      onload_state_(UNKNOWN),
      onload_millis_(-1),
      initialization_state_(INIT),
      viewport_width_(-1),
      viewport_height_(-1) {
}

PagespeedInput::PagespeedInput(ResourceFilter* resource_filter)
    : resources_(resource_filter),
      input_info_(new InputInformation),
      onload_state_(UNKNOWN),
      onload_millis_(-1),
      initialization_state_(INIT) {
}

PagespeedInput::~PagespeedInput() {
  STLDeleteContainerPointers(timeline_data_.begin(), timeline_data_.end());
}

bool PagespeedInput::AddResource(Resource* resource) {
  return resources_.AddResource(resource);
}

bool PagespeedInput::SetPrimaryResourceUrl(const std::string& url) {
  if (is_frozen()) {
    LOG(DFATAL) << "Can't set primary resource " << url
                << " to frozen PagespeedInput.";
    return false;
  }
  std::string canon_url = url;
  uri_util::CanonicalizeUrl(&canon_url);
  if (!has_resource_with_url(canon_url)) {
    LOG(INFO) << "No such primary resource " << canon_url;
    return false;
  }
  primary_resource_url_ = canon_url;
  return true;
}

bool PagespeedInput::SetOnloadState(OnloadState state) {
  if (is_frozen()) {
    LOG(DFATAL) << "Can't set onload state for frozen PagespeedInput.";
    return false;
  }
  onload_state_ = state;
  return true;
}

bool PagespeedInput::SetOnloadTimeMillis(int onload_millis) {
  if (is_frozen()) {
    LOG(DFATAL) << "Can't set onload time for frozen PagespeedInput.";
    return false;
  }
  if (onload_millis < 0) {
    LOG(DFATAL) << "Invalid onload_millis: " << onload_millis;
    return false;
  }
  onload_state_ = ONLOAD_FIRED;
  onload_millis_ = onload_millis;
  return true;
}

bool PagespeedInput::SetClientCharacteristics(const ClientCharacteristics& cc) {
  if (is_frozen()) {
    LOG(DFATAL) << "Can't set ClientCharacteristics for frozen PagespeedInput.";
    return false;
  }
  input_info_->mutable_client_characteristics()->CopyFrom(cc);
  return true;
}

bool PagespeedInput::SetViewportWidthAndHeight(int width, int height) {
  if (is_frozen()) {
    LOG(DFATAL) << "Can't set viewport for frozen PagespeedInput.";
    return false;
  }
  viewport_width_ = width;
  viewport_height_ = height;
  return true;
}

bool PagespeedInput::AcquireDomDocument(DomDocument* document) {
  if (is_frozen()) {
    LOG(DFATAL) << "Can't set DomDocument for frozen PagespeedInput.";
    return false;
  }
  document_.reset(document);
  return true;
}

bool PagespeedInput::AcquireImageAttributesFactory(
    ImageAttributesFactory *factory) {
  if (is_frozen()) {
    LOG(DFATAL)
        << "Can't set ImageAttributesFactory for frozen PagespeedInput.";
    return false;
  }
  image_attributes_factory_.reset(factory);
  return true;
}

bool PagespeedInput::AcquireInstrumentationData(
    InstrumentationDataVector* data) {
  if (is_frozen()) {
    LOG(DFATAL)
        << "Can't set InstrumentationDataVector for frozen PagespeedInput.";
    return false;
  }
  if (!timeline_data_.empty()) {
    LOG(DFATAL) << "Can't set InstrumentationDataVector. Already set.";
    return false;
  }
  timeline_data_.swap(*data);
  return true;
}

bool PagespeedInput::AcquireTopLevelBrowsingContext(
    TopLevelBrowsingContext* context) {
  if (is_frozen()) {
    LOG(DFATAL)
        << "Can't set top level browsing context for frozen PagespeedInput.";
    return false;
  }
  top_level_browsing_context_.reset(context);
  return true;
}

bool PagespeedInput::Freeze(
    PagespeedInputFreezeParticipant* freezeParticipant) {
  if (is_frozen()) {
    LOG(DFATAL) << "Can't Freeze frozen PagespeedInput.";
    return false;
  }
  initialization_state_ = FINALIZE;
  std::map<const Resource*, ResourceType> resource_type_map;
  PopulateResourceInformationFromDom(&resource_type_map);
  UpdateResourceTypes(resource_type_map);

  if (freezeParticipant != NULL) {
    freezeParticipant->OnFreeze(this);
  }

  resources_.Freeze();
  PopulateInputInformation();

  if (top_level_browsing_context_ != NULL) {
    if (!top_level_browsing_context_->Finalize()) {
      return false;
    }

    // TODO(michschn): Add a validator here to ensure that all BrowsingContexts,
    // ResourceFetches and ResourceEvaluations meet the expectations.
  }

  initialization_state_ = FROZEN;
  return true;
}

void PagespeedInput::PopulateInputInformation() {
  for (int idx = 0, num = num_resources(); idx < num; ++idx) {
    const Resource& resource = GetResource(idx);

    // Update input information
    int request_bytes = resource_util::EstimateRequestBytes(resource);
    input_info_->set_total_request_bytes(
        input_info_->total_request_bytes() + request_bytes);
    int response_bytes = resource_util::EstimateResponseBytes(resource);
    switch (resource.GetResourceType()) {
      case HTML:
        input_info_->set_html_response_bytes(
            input_info_->html_response_bytes() + response_bytes);
        break;
      case TEXT:
        input_info_->set_text_response_bytes(
            input_info_->text_response_bytes() + response_bytes);
        break;
      case CSS:
        input_info_->set_css_response_bytes(
            input_info_->css_response_bytes() + response_bytes);
        input_info_->set_number_css_resources(
            input_info_->number_css_resources() + 1);
        break;
      case IMAGE:
        input_info_->set_image_response_bytes(
            input_info_->image_response_bytes() + response_bytes);
        break;
      case JS:
        input_info_->set_javascript_response_bytes(
            input_info_->javascript_response_bytes() + response_bytes);
        input_info_->set_number_js_resources(
            input_info_->number_js_resources() + 1);
        break;
      case FLASH:
        input_info_->set_flash_response_bytes(
            input_info_->flash_response_bytes() + response_bytes);
        break;
      case REDIRECT:
      case OTHER:
        input_info_->set_other_response_bytes(
            input_info_->other_response_bytes() + response_bytes);
        break;
      case MEDIA:
        input_info_->set_media_response_bytes(
            input_info_->media_response_bytes() + response_bytes);
        break;
      default:
        LOG(DFATAL) << "Unknown resource type " << resource.GetResourceType();
        input_info_->set_other_response_bytes(
            input_info_->other_response_bytes() + response_bytes);
        break;
    }
    input_info_->set_number_resources(num_resources());
    input_info_->set_number_hosts(GetHostResourceMap()->size());
    if (resource_util::IsLikelyStaticResource(resource)) {
      input_info_->set_number_static_resources(
          input_info_->number_static_resources() + 1);
    }
  }
}

// DomElementVisitor that walks the DOM looking for nodes that
// reference external resources (e.g. <img src="foo.gif">).
class ExternalResourceNodeVisitor : public pagespeed::DomElementVisitor {
 public:
  ExternalResourceNodeVisitor(
      const pagespeed::PagespeedInput* pagespeed_input,
      const pagespeed::DomDocument* document,
      std::map<const Resource*, ResourceType>* resource_type_map)
      : pagespeed_input_(pagespeed_input),
        document_(document),
        resource_type_map_(resource_type_map) {
    SetUp();
  }

  virtual void Visit(const pagespeed::DomElement& node);

 private:
  void SetUp();

  void ProcessUri(const std::string& relative_uri,
                  ResourceType type);

  const pagespeed::PagespeedInput* pagespeed_input_;
  const pagespeed::DomDocument* document_;
  std::map<const Resource*, ResourceType>* resource_type_map_;
  ResourceSet visited_resources_;

  DISALLOW_COPY_AND_ASSIGN(ExternalResourceNodeVisitor);
};

void ExternalResourceNodeVisitor::ProcessUri(const std::string& relative_uri,
                                             ResourceType type) {
  if (relative_uri.empty()) {
    // An empty URI gets resolved to the URI of its parent document,
    // which will cause us to change the type of the parent
    // document. This is not the intended effect so we skip over empty
    // URIs.
    return;
  }
  std::string uri = document_->ResolveUri(relative_uri);
  if (!uri_util::IsExternalResourceUrl(uri)) {
    // If this is a URL for a non-external resource (e.g. a data URI)
    // then we should not attempt to process it.
    return;
  }
  const Resource* resource = pagespeed_input_->GetResourceWithUrlOrNull(uri);
  if (resource == NULL) {
    LOG(INFO) << "Unable to find resource " << uri;
    return;
  }

  if (resource->GetResourceType() == REDIRECT) {
    resource = resource_util::GetLastResourceInRedirectChain(
        *pagespeed_input_, *resource);
    if (resource == NULL) {
      return;
    }
  }

  // Update the Resource->ResourceType map.
  if (type != OTHER) {
    std::map<const Resource*, ResourceType>::const_iterator it =
        resource_type_map_->find(resource);
    if (it != resource_type_map_->end()) {
      ResourceType existing_type = it->second;
      if (existing_type != type) {
        LOG(INFO) << "Multiple ResourceTypes for " << resource->GetRequestUrl();
      }
    } else {
      (*resource_type_map_)[resource] = type;
    }
  }
}

void ExternalResourceNodeVisitor::SetUp() {
}

void ExternalResourceNodeVisitor::Visit(const pagespeed::DomElement& node) {
  if (node.GetTagName() == "IMG" ||
      node.GetTagName() == "SCRIPT" ||
      node.GetTagName() == "IFRAME" ||
      node.GetTagName() == "EMBED") {
    std::string src;
    if (node.GetAttributeByName("src", &src)) {
      ResourceType type;
      if (node.GetTagName() == "IMG") {
        type = IMAGE;
      } else if (node.GetTagName() == "SCRIPT") {
        type = JS;
      } else if (node.GetTagName() == "IFRAME") {
        type = HTML;
      } else if (node.GetTagName() == "EMBED") {
        // TODO(bmcquade): in some cases this resource may be flash,
        // but not always. Thus we set type to OTHER. ProcessUri
        // ignores type OTHER but will update the document child
        // resource map, which is what we want.
        type = OTHER;
      } else {
        LOG(DFATAL) << "Unexpected type " << node.GetTagName();
        type = OTHER;
      }
      ProcessUri(src, type);
    }
  } else if (node.GetTagName() == "LINK") {
    std::string rel;
    if (node.GetAttributeByName("rel", &rel) &&
        LowerCaseEqualsASCII(rel, "stylesheet")) {
      std::string href;
      if (node.GetAttributeByName("href", &href)) {
        ProcessUri(href, CSS);
      }
    }
  }

  if (node.GetTagName() == "IFRAME") {
    // Do a recursive document traversal.
    scoped_ptr<pagespeed::DomDocument> child_doc(node.GetContentDocument());
    if (child_doc.get()) {
      ExternalResourceNodeVisitor visitor(pagespeed_input_,
                                          child_doc.get(),
                                          resource_type_map_);
      child_doc->Traverse(&visitor);
    }
  }
}

void PagespeedInput::PopulateResourceInformationFromDom(
    std::map<const Resource*, ResourceType>* resource_type_map) {
  if (dom_document() != NULL) {
    ExternalResourceNodeVisitor visitor(this,
                                        dom_document(),
                                        resource_type_map);
    dom_document()->Traverse(&visitor);
  }
}

void PagespeedInput::UpdateResourceTypes(
    const std::map<const Resource*, ResourceType>& resource_type_map) {
  for (int idx = 0, num = num_resources(); idx < num; ++idx) {
    Resource* resource = resources_.GetMutableResource(idx);
    std::map<const Resource*, ResourceType>::const_iterator it =
        resource_type_map.find(resource);
    if (it != resource_type_map.end()) {
      resource->SetResourceType(it->second);
    }
  }
}

const ResourceCollection& PagespeedInput::GetResourceCollection() const {
  return resources_;
}

int PagespeedInput::num_resources() const {
  return resources_.num_resources();
}

bool PagespeedInput::has_resource_with_url(const std::string& url) const {
  return resources_.has_resource_with_url(url);
}

const Resource& PagespeedInput::GetResource(int idx) const {
  return resources_.GetResource(idx);
}

ImageAttributes* PagespeedInput::NewImageAttributes(
    const Resource* resource) const {
  DCHECK(initialization_state_ != INIT);
  if (image_attributes_factory_ == NULL) {
    return NULL;
  }
  return image_attributes_factory_->NewImageAttributes(resource);
}

const TopLevelBrowsingContext*
PagespeedInput::GetTopLevelBrowsingContext() const {
  return top_level_browsing_context_.get();
}

TopLevelBrowsingContext*
PagespeedInput::GetMutableTopLevelBrowsingContext() {
  if (is_frozen()) {
    LOG(DFATAL) << "Unable to get mutable top level browsing context "
        "after freezing.";
    return NULL;
  }
  return top_level_browsing_context_.get();
}

const HostResourceMap* PagespeedInput::GetHostResourceMap() const {
  return resources_.GetHostResourceMap();
}

const ResourceVector*
PagespeedInput::GetResourcesInRequestOrder() const {
  return resources_.GetResourcesInRequestOrder();
}

const InputInformation* PagespeedInput::input_information() const {
  DCHECK(initialization_state_ != INIT);
  return input_info_.get();
}

const DomDocument* PagespeedInput::dom_document() const {
  DCHECK(initialization_state_ != INIT);
  return document_.get();
}

const InstrumentationDataVector* PagespeedInput::instrumentation_data() const {
  DCHECK(initialization_state_ != INIT);
  return &timeline_data_;
}

const std::string& PagespeedInput::primary_resource_url() const {
  return primary_resource_url_;
}

bool PagespeedInput::is_frozen() const {
  return initialization_state_ == FROZEN;
}

bool PagespeedInput::IsResourceLoadedAfterOnload(
    const Resource& resource) const {
  if (onload_state_ != ONLOAD_FIRED) {
    // If we don't have an onload time, assume the resource is not
    // loaded after onload.
    return false;
  }
  if (onload_millis_ < 0) {
    LOG(DFATAL)
        << "onload_state_ is ONLOAD_FIRED but no onload time specified.";
    return false;
  }
  if (!resource.has_request_start_time_millis()) {
    // If no request start time, assume it's not loaded after onload.
    return false;
  }
  return resource.request_start_time_millis_ > onload_millis_;
}

const Resource* PagespeedInput::GetResourceWithUrlOrNull(
    const std::string& url) const {
  return resources_.GetResourceWithUrlOrNull(url);
}

Resource* PagespeedInput::GetMutableResource(int idx) {
  return resources_.GetMutableResource(idx);
}

Resource* PagespeedInput::GetMutableResourceWithUrlOrNull(
    const std::string& url) {
  return resources_.GetMutableResourceWithUrlOrNull(url);
}

InputCapabilities PagespeedInput::EstimateCapabilities() const {
  InputCapabilities capabilities;
  if (!is_frozen()) {
    LOG(DFATAL) << "Can't estimate capabilities of non-frozen input.";
    return capabilities;
  }

  if (resources_.num_resources() == 0) {
    // No resources means we have nothing with which to compute
    // capabilities.
    return capabilities;
  }

  if (dom_document() != NULL) {
    capabilities.add(InputCapabilities::DOM);
  }
  if (!timeline_data_.empty()) {
    capabilities.add(InputCapabilities::TIMELINE_DATA);
  }
  if (GetResourcesInRequestOrder() != NULL) {
    capabilities.add(InputCapabilities::REQUEST_START_TIMES);
  }
  if (onload_state_ != UNKNOWN) {
    capabilities.add(InputCapabilities::ONLOAD);
  }
  for (int i = 0, num = num_resources(); i < num; ++i) {
    const Resource& resource = GetResource(i);
    if (!resource.GetResponseBody().empty()) {
      capabilities.add(InputCapabilities::RESPONSE_BODY);
    }
    if (!resource.GetRequestHeader("referer").empty() &&
        !resource.GetRequestHeader("host").empty() &&
        !resource.GetRequestHeader("accept-encoding").empty()) {
      // If at least one resource has a Host, Referer, and
      // Accept-Encoding header, we assume that a full set of request
      // headers were provided.
      capabilities.add(InputCapabilities::REQUEST_HEADERS);
    }
  }

  if (top_level_browsing_context_ != NULL) {
    // If at least one resource in the top level browsing context has a
    // ResourceFetch or ResourceEvaluation associated, we assume the dependency
    // information has been calculated.
    ResourceVector context_resources;
    top_level_browsing_context_->GetResources(&context_resources);
    for (ResourceVector::const_iterator it = context_resources.begin();
        it != context_resources.end(); ++it) {
      if (top_level_browsing_context_->GetResourceFetchCount(**it) != 0 ||
          top_level_browsing_context_->GetResourceEvaluationCount(**it) != 0) {
        capabilities.add(InputCapabilities::DEPENDENCY_DATA);
        break;
      }
    }
  }
  return capabilities;
}

}  // namespace pagespeed
