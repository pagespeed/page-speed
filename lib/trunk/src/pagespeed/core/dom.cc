/**
 * Copyright 2009 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "dom.h"

#include "base/logging.h"
#include "base/scoped_ptr.h"
#include "base/string_util.h"
#include "pagespeed/core/uri_util.h"

#define NOT_IMPLEMENTED() do {                          \
    LOG(WARNING) << __FUNCTION__ << " not implemented"; \
  } while (false)

namespace pagespeed {

namespace {

class ExternalResourceVisitorAdaptor : public DomElementVisitor {
 public:
  ExternalResourceVisitorAdaptor(
      ExternalResourceDomElementVisitor* inner, const DomDocument* document);

  virtual void Visit(const DomElement& node);

 private:
  ExternalResourceDomElementVisitor* inner_;
  const DomDocument* document_;

  DISALLOW_COPY_AND_ASSIGN(ExternalResourceVisitorAdaptor);
};

ExternalResourceVisitorAdaptor::ExternalResourceVisitorAdaptor(
    ExternalResourceDomElementVisitor* inner, const DomDocument* document)
    : inner_(inner), document_(document) {
}

void ExternalResourceVisitorAdaptor::Visit(const DomElement& node) {
  bool found_uri = false;
  std::string relative_uri;
  if (node.GetTagName() == "IMG" ||
      node.GetTagName() == "SCRIPT" ||
      node.GetTagName() == "IFRAME" ||
      node.GetTagName() == "EMBED") {
    found_uri = node.GetAttributeByName("src", &relative_uri);
  } else if (node.GetTagName() == "LINK") {
    std::string rel;
    if (node.GetAttributeByName("rel", &rel) &&
        LowerCaseEqualsASCII(rel, "stylesheet")) {
      found_uri = node.GetAttributeByName("href", &relative_uri);
    }
  }
  if (found_uri && !relative_uri.empty()) {
    std::string resolved_uri =
        pagespeed::uri_util::ResolveUri(relative_uri, document_->GetBaseUrl());
    if (uri_util::IsExternalResourceUrl(resolved_uri)) {
      inner_->VisitUrl(node, resolved_uri);
    }
  }

  if (node.GetTagName() == "IFRAME") {
    scoped_ptr<DomDocument> document(node.GetContentDocument());
    if (document != NULL) {
      inner_->VisitDocument(node, *document.get());
    }
  }
}

}  // namespace

DomDocument::DomDocument() {}

DomDocument::~DomDocument() {}

DomDocument::Status DomDocument::GetWidth(int* out_width) const {
  NOT_IMPLEMENTED();
  return FAILURE;
}

DomDocument::Status DomDocument::GetHeight(int* out_height) const {
  NOT_IMPLEMENTED();
  return FAILURE;
}

std::string DomDocument::ResolveUri(const std::string& uri) const {
  return pagespeed::uri_util::ResolveUri(uri, GetBaseUrl());
}

DomElement::DomElement() {}

DomElement::~DomElement() {}

DomElement::Status DomElement::GetX(int* out_x) const {
  NOT_IMPLEMENTED();
  return FAILURE;
}

DomElement::Status DomElement::GetY(int* out_y) const {
  NOT_IMPLEMENTED();
  return FAILURE;
}

DomElement::Status DomElement::GetActualWidth(int* out_width) const {
  NOT_IMPLEMENTED();
  return FAILURE;
}

DomElement::Status DomElement::GetActualHeight(int* out_height) const {
  NOT_IMPLEMENTED();
  return FAILURE;
}

DomElement::Status DomElement::HasWidthSpecified(
    bool* out_width_specified) const {
  NOT_IMPLEMENTED();
  return FAILURE;
}

DomElement::Status DomElement::HasHeightSpecified(
    bool* out_height_specified) const {
  NOT_IMPLEMENTED();
  return FAILURE;
}

DomElementVisitor::DomElementVisitor() {}

DomElementVisitor::~DomElementVisitor() {}

ExternalResourceDomElementVisitor::ExternalResourceDomElementVisitor() {}
ExternalResourceDomElementVisitor::~ExternalResourceDomElementVisitor() {}

DomElementVisitor* MakeDomElementVisitorForDocument(
    const DomDocument* document,
    ExternalResourceDomElementVisitor* visitor) {
  return new ExternalResourceVisitorAdaptor(visitor, document);
}

DomRect DomRect::Intersection(const DomRect& other) const {
  int left = std::max(x(), other.x());
  int top = std::max(y(), other.y());
  int right = std::min(x() + width(), other.x() + other.width());
  int bottom = std::min(y() + height(), other.y() + other.height());
  int width = std::max(0, right - left);
  int height = std::max(0, bottom - top);
  return DomRect(left, top, width, height);
}

}  // namespace pagespeed
