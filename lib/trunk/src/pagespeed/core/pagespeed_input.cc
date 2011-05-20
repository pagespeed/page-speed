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
#include "base/string_util.h"
#include "pagespeed/core/dom.h"
#include "pagespeed/core/image_attributes.h"
#include "pagespeed/core/resource.h"
#include "pagespeed/core/resource_util.h"
#include "pagespeed/core/uri_util.h"
#include "pagespeed/proto/pagespeed_output.pb.h"

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
      resource_filter_(new AllowAllResourceFilter),
      onload_state_(UNKNOWN),
      onload_millis_(-1),
      frozen_(false) {
}

PagespeedInput::PagespeedInput(ResourceFilter* resource_filter)
    : input_info_(new InputInformation),
      resource_filter_(resource_filter),
      onload_state_(UNKNOWN),
      onload_millis_(-1),
      frozen_(false) {
  DCHECK_NE(resource_filter, static_cast<ResourceFilter*>(NULL));
}

PagespeedInput::~PagespeedInput() {
  STLDeleteContainerPointers(resources_.begin(), resources_.end());
}

bool PagespeedInput::IsValidResource(const Resource* resource) const {
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

  // TODO: consider adding some basic validation for request/response
  // headers.

  return true;
}

bool PagespeedInput::AddResource(Resource* resource) {
  if (frozen_) {
    LOG(DFATAL) << "Can't add resource " << resource->GetRequestUrl()
                << " to frozen PagespeedInput.";
    delete resource;  // Resource is owned by PagespeedInput.
    return false;
  }
  if (!IsValidResource(resource)) {
    delete resource;  // Resource is owned by PagespeedInput.
    return false;
  }
  const std::string& url = resource->GetRequestUrl();

  resources_.push_back(resource);
  url_resource_map_[url] = resource;
  host_resource_map_[resource->GetHost()].insert(resource);
  return true;
}

bool PagespeedInput::SetPrimaryResourceUrl(const std::string& url) {
  if (frozen_) {
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
  if (frozen_) {
    LOG(DFATAL) << "Can't set onload state for frozen PagespeedInput.";
    return false;
  }
  onload_state_ = state;
  return true;
}

bool PagespeedInput::SetOnloadTimeMillis(int onload_millis) {
  if (frozen_) {
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
  if (frozen_) {
    LOG(DFATAL) << "Can't set ClientCharacteristics for frozen PagespeedInput.";
    return false;
  }
  input_info_->mutable_client_characteristics()->CopyFrom(cc);
  return true;
}

bool PagespeedInput::AcquireDomDocument(DomDocument* document) {
  if (frozen_) {
    LOG(DFATAL) << "Can't set DomDocument for frozen PagespeedInput.";
    return false;
  }
  document_.reset(document);
  return true;
}

bool PagespeedInput::AcquireImageAttributesFactory(
    ImageAttributesFactory *factory) {
  if (frozen_) {
    LOG(DFATAL)
        << "Can't set ImageAttributesFactory for frozen PagespeedInput.";
    return false;
  }
  image_attributes_factory_.reset(factory);
  return true;
}

bool PagespeedInput::Freeze() {
  if (frozen_) {
    LOG(DFATAL) << "Can't Freeze frozen PagespeedInput.";
    return false;
  }
  frozen_ = true;
  std::map<const Resource*, ResourceType> resource_type_map;
  PopulateResourceInformationFromDom(
      &resource_type_map, &parent_child_resource_map_);
  UpdateResourceTypes(resource_type_map);
  PopulateInputInformation();
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
      std::map<const Resource*, ResourceType>* resource_type_map,
      ParentChildResourceMap* parent_child_resource_map)
      : pagespeed_input_(pagespeed_input),
        document_(document),
        resource_type_map_(resource_type_map),
        parent_child_resource_map_(parent_child_resource_map) {
    SetUp();
  }

  virtual void Visit(const pagespeed::DomElement& node);

 private:
  void SetUp();

  void ProcessUri(const std::string& relative_uri, ResourceType type);

  const pagespeed::PagespeedInput* pagespeed_input_;
  const pagespeed::DomDocument* document_;
  std::map<const Resource*, ResourceType>* resource_type_map_;
  ParentChildResourceMap* parent_child_resource_map_;
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

  // Update the Parent->Child resource map.
  const Resource* document_resource =
      pagespeed_input_->GetResourceWithUrlOrNull(document_->GetDocumentUrl());
  if (document_resource != NULL) {
    if (visited_resources_.count(resource) == 0) {
      // Only insert the resource into the vector once.
      visited_resources_.insert(resource);
      (*parent_child_resource_map_)[document_resource].push_back(resource);
    }
  } else {
    LOG(INFO) << "Unable to find resource for " << document_->GetDocumentUrl();
  }
}

void ExternalResourceNodeVisitor::SetUp() {
  const Resource* document_resource =
      pagespeed_input_->GetResourceWithUrlOrNull(document_->GetDocumentUrl());
  if (document_resource != NULL) {
    // Create an initial entry in the parent_child_resource_map.
    (*parent_child_resource_map_)[document_resource];
  }
}

void ExternalResourceNodeVisitor::Visit(const pagespeed::DomElement& node) {
  if (node.GetTagName() == "IMG" ||
      node.GetTagName() == "SCRIPT" ||
      node.GetTagName() == "IFRAME" ||
      node.GetTagName() == "EMBED") {
    // NOTE: an iframe created/manipulated via JS may not have a "src"
    // attribute but can still have children. We should handle this
    // case. This most likely requires redefining the
    // ParentChildResourceMap structure.
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
        // TODO: in some cases this resource may be flash, but not
        // always. Thus we set type to OTHER. ProcessUri ignores type
        // OTHER but will update the ParentChildResourceMap, which is
        // what we want.
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
                                          resource_type_map_,
                                          parent_child_resource_map_);
      child_doc->Traverse(&visitor);
    }
  }
}

void PagespeedInput::PopulateResourceInformationFromDom(
    std::map<const Resource*, ResourceType>* resource_type_map,
    ParentChildResourceMap* parent_child_resource_map) {
  if (dom_document() != NULL) {
    ExternalResourceNodeVisitor visitor(this,
                                        dom_document(),
                                        resource_type_map,
                                        parent_child_resource_map);
    dom_document()->Traverse(&visitor);
  }
}

void PagespeedInput::UpdateResourceTypes(
    const std::map<const Resource*, ResourceType>& resource_type_map) {
  for (int idx = 0, num = num_resources(); idx < num; ++idx) {
    Resource* resource = resources_[idx];
    std::map<const Resource*, ResourceType>::const_iterator it =
        resource_type_map.find(resource);
    if (it != resource_type_map.end()) {
      resource->SetResourceType(it->second);
    }
  }
}

int PagespeedInput::num_resources() const {
  return resources_.size();
}

bool PagespeedInput::has_resource_with_url(const std::string& url) const {
  std::string url_canon;
  if (!uri_util::GetUriWithoutFragment(url, &url_canon)) {
    url_canon = url;
  }
  return url_resource_map_.find(url_canon) != url_resource_map_.end();
}

const Resource& PagespeedInput::GetResource(int idx) const {
  DCHECK(idx >= 0 && static_cast<size_t>(idx) < resources_.size());
  return *resources_[idx];
}

ImageAttributes* PagespeedInput::NewImageAttributes(
    const Resource* resource) const {
  DCHECK(frozen_);
  if (image_attributes_factory_ == NULL) {
    return NULL;
  }
  return image_attributes_factory_->NewImageAttributes(resource);
}

const HostResourceMap* PagespeedInput::GetHostResourceMap() const {
  DCHECK(frozen_);
  return &host_resource_map_;
}

const ResourceVector*
PagespeedInput::GetResourcesInRequestOrder() const {
  DCHECK(frozen_);
  if (request_order_vector_.empty()) return NULL;
  DCHECK(request_order_vector_.size() == resources_.size());
  return &request_order_vector_;
}

const ParentChildResourceMap*
PagespeedInput::GetParentChildResourceMap() const {
  DCHECK(frozen_);
  return &parent_child_resource_map_;
}


const InputInformation* PagespeedInput::input_information() const {
  DCHECK(frozen_);
  return input_info_.get();
}

const DomDocument* PagespeedInput::dom_document() const {
  DCHECK(frozen_);
  return document_.get();
}

const std::string& PagespeedInput::primary_resource_url() const {
  return primary_resource_url_;
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

Resource* PagespeedInput::GetMutableResource(int idx) {
  if (frozen_) {
    LOG(DFATAL) << "Unable to get mutable resource after freezing.";
    return NULL;
  }
  return const_cast<Resource*>(&GetResource(idx));
}

Resource* PagespeedInput::GetMutableResourceWithUrl(
    const std::string& url) {
  if (frozen_) {
    LOG(DFATAL) << "Unable to get mutable resource after freezing.";
    return NULL;
  }
  return const_cast<Resource*>(GetResourceWithUrlOrNull(url));
}

InputCapabilities PagespeedInput::EstimateCapabilities() const {
  InputCapabilities capabilities;
  if (!is_frozen()) {
    LOG(DFATAL) << "Can't estimate capabilities of non-frozen input.";
    return capabilities;
  }

  if (resources_.empty()) {
    // No resources means we have nothing with which to compute
    // capabilities.
    return capabilities;
  }

  if (dom_document() != NULL) {
    capabilities.add(
        InputCapabilities::DOM |
        InputCapabilities::PARENT_CHILD_RESOURCE_MAP);
  }
  if (GetResourcesInRequestOrder() != NULL) {
    capabilities.add(InputCapabilities::REQUEST_START_TIMES);
  }
  if (onload_state_ != UNKNOWN) {
    capabilities.add(InputCapabilities::ONLOAD);
  }
  for (int i = 0, num = num_resources(); i < num; ++i) {
    const Resource& resource = GetResource(i);
    if (resource.GetJavaScriptCalls("document.write") != NULL) {
      capabilities.add(InputCapabilities::JS_CALLS_DOCUMENT_WRITE);
    }
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
  return capabilities;
}

}  // namespace pagespeed
