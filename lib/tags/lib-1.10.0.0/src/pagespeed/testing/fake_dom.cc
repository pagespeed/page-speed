// Copyright 2010 Google Inc. All Rights Reserved.
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

#include "pagespeed/testing/fake_dom.h"

#include "base/stl_util-inl.h"  // for STLDeleteContainerPointers
#include "base/string_util.h"

namespace {

// Helper class that performs a pre-order traversal from the given
// root FakeDomElement.
class PreOrderFakeElementTraverser {
 public:
  explicit PreOrderFakeElementTraverser(
      const pagespeed_testing::FakeDomElement* root);

  bool NextElement();
  const pagespeed_testing::FakeDomElement* CurrentElement() { return element_; }

 private:
  // We need to store the root, in addition to a pointer to the
  // current element. Otherwise, we would end up iterating through the
  // parents of root_, if root is not the actual root of the DOM.
  const pagespeed_testing::FakeDomElement* const root_;
  const pagespeed_testing::FakeDomElement* element_;

  DISALLOW_COPY_AND_ASSIGN(PreOrderFakeElementTraverser);
};

PreOrderFakeElementTraverser::PreOrderFakeElementTraverser(
    const pagespeed_testing::FakeDomElement* root)
    : root_(root), element_(root) {
}

bool PreOrderFakeElementTraverser::NextElement() {
  // First, if the element has a child, visit the child.
  if (element_ == NULL) {
    return false;
  }

  const pagespeed_testing::FakeDomElement* next = element_->GetFirstChild();
  if (next != NULL) {
    element_ = next;
    return true;
  }

  // We need to traverse siblings, walking up the parent chain until
  // we find a valid sibling.
  for (next = element_;
       next != NULL && next != root_;
       next = next->GetParentElement()) {
    const pagespeed_testing::FakeDomElement* sibling = next->GetNextSibling();
    if (sibling != NULL) {
      element_ = sibling;
      return true;
    }
  }

  element_ = NULL;
  return false;
}

}  // namespace

namespace pagespeed_testing {

// static
FakeDomElement* FakeDomElement::NewIframe(FakeDomElement* parent) {
  FakeDomElement* iframe = new FakeDomElement(parent, "iframe");
  parent->children_.push_back(iframe);
  return iframe;
}

// static
FakeDomElement* FakeDomElement::NewRoot(FakeDomDocument* parent,
                                        const std::string& tag_name) {
  if (parent->document_element_ != NULL) {
    LOG(DFATAL) << "Document already has document element.";
    return NULL;
  }
  FakeDomElement* element = new FakeDomElement(NULL, tag_name);
  parent->document_element_ = element;
  return element;
}

// static
FakeDomElement* FakeDomElement::New(FakeDomElement* parent,
                                    const std::string& tag_name) {
  FakeDomElement* element = new FakeDomElement(parent, tag_name);
  parent->children_.push_back(element);
  return element;
}

// static
FakeDomElement* FakeDomElement::NewImg(FakeDomElement* parent,
                                       const std::string& url) {
  FakeDomElement* img = FakeDomElement::New(parent, "img");
  img->AddAttribute("src", url);
  return img;
}

// static
FakeDomElement* FakeDomElement::NewScript(FakeDomElement* parent,
                                          const std::string& url) {
  FakeDomElement* img = FakeDomElement::New(parent, "script");
  img->AddAttribute("src", url);
  return img;
}

// static
FakeDomElement* FakeDomElement::NewStyle(FakeDomElement* parent) {
  return FakeDomElement::New(parent, "style");
}

// static
FakeDomElement* FakeDomElement::NewLinkStylesheet(FakeDomElement* parent,
                                                  const std::string& url) {
  FakeDomElement* link = FakeDomElement::New(parent, "link");
  link->AddAttribute("rel", "stylesheet");
  link->AddAttribute("href", url);
  return link;
}

FakeDomElement::FakeDomElement(const FakeDomElement* parent,
                               const std::string& tag_name)
    : tag_name_(tag_name),
      parent_(parent),
      document_(NULL),
      actual_width_(-1),
      actual_height_(-1) {
  StringToUpperASCII(&tag_name_);
}

FakeDomElement::~FakeDomElement() {
  STLDeleteContainerPointers(children_.begin(), children_.end());
  if (document_ != NULL) {
    delete document_;
  }
}

pagespeed::DomDocument* FakeDomElement::GetContentDocument() const {
  if (tag_name_ != "IFRAME") {
    LOG(DFATAL) << "No content document for non-iframe element.";
    return NULL;
  }
  if (document_ == NULL) {
    LOG(INFO) << "No document available.";
    return NULL;
  }
  return document_->Clone();
}

std::string FakeDomElement::GetTagName() const {
  return tag_name_;
}

bool FakeDomElement::GetAttributeByName(const std::string& name,
                                        std::string* attr_value) const {
  StringStringMap::const_iterator it = attributes_.find(name);
  if (it != attributes_.end()) {
    *attr_value = it->second;
    return true;
  }
  return false;
}

void FakeDomElement::AddAttribute(const std::string& key,
                                  const std::string& value) {
  attributes_[key] = value;
}

void FakeDomElement::RemoveAttribute(const std::string& key) {
  attributes_.erase(key);
}

void FakeDomElement::SetActualWidthAndHeight(int width, int height) {
  actual_width_ = width;
  actual_height_ = height;
}

const FakeDomElement* FakeDomElement::GetFirstChild() const {
  if (children_.size() == 0) {
    return NULL;
  }
  return children_[0];
}

const FakeDomElement* FakeDomElement::GetParentElement() const {
  return parent_;
}

const FakeDomElement* FakeDomElement::GetNextSibling() const {
  if (parent_ == NULL) {
    return NULL;
  }

  for (ChildVector::const_iterator it = parent_->children_.begin(),
           end = parent_->children_.end();
       it != end;
       ++it) {
    if (*it == this) {
      // We found our entry in the parent's child list, so return the
      // next entry, if any.
      ++it;
      if (it != end) {
        return *it;
      } else {
        return NULL;
      }
    }
  }

  LOG(DFATAL) << "Unable to find this in parent's child vector.";
  return NULL;
}

// static
FakeDomDocument* FakeDomDocument::NewRoot(const std::string& document_url) {
  return new FakeDomDocument(document_url);
}

// static
FakeDomDocument* FakeDomDocument::New(FakeDomElement* iframe,
                                      const std::string& document_url) {
  if (iframe->GetTagName() != "IFRAME") {
    LOG(DFATAL) << "Unable to create document in non-iframe tag.";
    return NULL;
  }
  if (iframe->document_ != NULL) {
    LOG(DFATAL) << "iframe already has child document.";
    return NULL;
  }
  iframe->AddAttribute("src", document_url);
  FakeDomDocument* document = new FakeDomDocument(document_url);
  iframe->document_ = document;
  return document;
}

FakeDomDocument::FakeDomDocument(const std::string& document_url)
    : url_(document_url),
      document_element_(NULL),
      is_clone_(false) {
}

FakeDomDocument::~FakeDomDocument() {
  if (!is_clone_) {
    delete document_element_;
  }
}

std::string FakeDomDocument::GetDocumentUrl() const {
  return url_;
}

std::string FakeDomDocument::GetBaseUrl() const {
  if (!base_url_.empty()) {
    return base_url_;
  }
  return url_;
}

void FakeDomDocument::Traverse(pagespeed::DomElementVisitor* visitor) const {
  PreOrderFakeElementTraverser traverser(GetDocumentElement());
  if (traverser.CurrentElement() == NULL) {
    return;
  }
  do {
    const pagespeed_testing::FakeDomElement* element =
        traverser.CurrentElement();
    visitor->Visit(*element);
  } while (traverser.NextElement());
}

pagespeed::DomElement::Status FakeDomElement::GetActualWidth(
    int* out_width) const {
  if (actual_width_ >= 0) {
    *out_width = actual_width_;
    return SUCCESS;
  }
  return FAILURE;
}

pagespeed::DomElement::Status FakeDomElement::GetActualHeight(
    int* out_height) const {
  if (actual_height_ >= 0) {
    *out_height = actual_height_;
    return SUCCESS;
  }
  return FAILURE;
}

pagespeed::DomElement::Status FakeDomElement::HasHeightSpecified(
    bool *out) const {
  *out = attributes_.find("height") != attributes_.end();
  return SUCCESS;
}

pagespeed::DomElement::Status FakeDomElement::HasWidthSpecified(
    bool *out) const {
  *out = attributes_.find("width") != attributes_.end();
  return SUCCESS;
}

FakeDomDocument* FakeDomDocument::Clone() const {
  FakeDomDocument* clone = new FakeDomDocument(url_);
  clone->document_element_ = document_element_;
  clone->base_url_ = base_url_;
  clone->is_clone_ = true;
  return clone;
}

const FakeDomElement* FakeDomDocument::GetDocumentElement() const {
  return document_element_;
}

}  // namespace pagespeed_testing
