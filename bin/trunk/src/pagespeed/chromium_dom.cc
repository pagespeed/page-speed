// Copyright 2010 Google Inc.
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

#include <string>

#include "pagespeed/chromium_dom.h"
#include "pagespeed/core/dom.h"
#include "third_party/WebKit/WebKit/chromium/public/WebDocument.h"
#include "third_party/WebKit/WebKit/chromium/public/WebElement.h"
#include "third_party/WebKit/WebKit/chromium/public/WebFrame.h"
#include "third_party/WebKit/WebKit/chromium/public/WebString.h"

namespace pagespeed {

namespace {

class ChromiumDocument : public DomDocument {
 public:
  ChromiumDocument(const WebKit::WebDocument& document);

  virtual std::string GetDocumentUrl() const;
  virtual std::string GetBaseUrl() const;
  virtual void Traverse(DomElementVisitor* visitor) const;

 private:
  void DoTraverse(DomElementVisitor* visitor,
                  WebKit::WebElement element) const;

  const WebKit::WebDocument document_;

  DISALLOW_COPY_AND_ASSIGN(ChromiumDocument);
};

class ChromiumElement : public DomElement {
 public:
  ChromiumElement(const WebKit::WebElement& element);
  virtual DomDocument* GetContentDocument() const;
  virtual std::string GetTagName() const;
  virtual bool GetAttributeByName(const std::string& name,
                                  std::string* attr_value) const;

  Status GetActualWidth(int* out_width) const;
  Status GetActualHeight(int* out_height) const;
  Status HasWidthSpecified(bool* out_width_specified) const;
  Status HasHeightSpecified(bool* out_height_specified) const;

 private:
  const WebKit::WebElement element_;

  DISALLOW_COPY_AND_ASSIGN(ChromiumElement);
};

// Helper class that performs a pre-order traversal from the given
// root WebNode.
class PreOrderChromiumNodeTraverser {
 public:
  PreOrderChromiumNodeTraverser(WebKit::WebNode root);

  bool NextNode();
  WebKit::WebNode CurrentNode() { return node_; }

 private:
  // We need to store the root, in addition to a pointer to the
  // current node. Otherwise, we would end up iterating through the
  // parents of root_, if root is not the actual root of the DOM.
  const WebKit::WebNode root_;
  WebKit::WebNode node_;

  DISALLOW_COPY_AND_ASSIGN(PreOrderChromiumNodeTraverser);
};

ChromiumDocument::ChromiumDocument(const WebKit::WebDocument& document)
    : document_(document) {
}

std::string ChromiumDocument::GetDocumentUrl() const {
  return document_.frame()->url().spec();
}

std::string ChromiumDocument::GetBaseUrl() const {
  return document_.baseURL().spec();
}

void ChromiumDocument::Traverse(DomElementVisitor* visitor) const {
  PreOrderChromiumNodeTraverser traverser(document_.documentElement());
  do {
    WebKit::WebNode node = traverser.CurrentNode();
    if (!node.isElementNode()) {
      continue;
    }
    WebKit::WebElement element = node.toConst<WebKit::WebElement>();
    ChromiumElement e(element);
    visitor->Visit(e);
  } while (traverser.NextNode());
}

ChromiumElement::ChromiumElement(const WebKit::WebElement& element)
    : element_(element) {
}

DomDocument* ChromiumElement::GetContentDocument() const {
  if (!element_.hasTagName("frame") && !element_.hasTagName("iframe")) {
    return NULL;
  }

  WebKit::WebFrame* frame = WebKit::WebFrame::fromFrameOwnerElement(element_);
  return new ChromiumDocument(frame->document());
}

std::string ChromiumElement::GetTagName() const {
  return element_.tagName().utf8();
}

bool ChromiumElement::GetAttributeByName(const std::string& name,
                                         std::string* attr_value) const {
  WebKit::WebString name_str = WebKit::WebString::fromUTF8(name.c_str());
  if (!element_.hasAttribute(name_str)) {
    return false;
  }
  *attr_value = element_.getAttribute(name_str).utf8();
  return true;
}

DomElement::Status ChromiumElement::GetActualWidth(int* out_width) const {
  // TODO(bmcquade): find a way to get the width of an element.
  return FAILURE;
}

DomElement::Status ChromiumElement::GetActualHeight(int* out_height) const {
  // TODO(bmcquade): find a way to get the height of an element.
  return FAILURE;
}

DomElement::Status ChromiumElement::HasWidthSpecified(
    bool* out_width_specified) const {
  // TODO(bmcquade): find a way to find out whether the width is
  // explicitly specified (i.e. as an attribute, inline style, or via
  // CSS).
  return FAILURE;
}

DomElement::Status ChromiumElement::HasHeightSpecified(
    bool* out_height_specified) const {
  // TODO(bmcquade): find a way to find out whether the height is
  // explicitly specified (i.e. as an attribute, inline style, or via
  // CSS).
  return FAILURE;
}

PreOrderChromiumNodeTraverser::PreOrderChromiumNodeTraverser(
    const WebKit::WebNode root) : root_(root), node_(root) {
}

bool PreOrderChromiumNodeTraverser::NextNode() {
  // First, if the node has a child, visit the child.
  WebKit::WebNode next = node_.firstChild();
  if (!next.isNull()) {
    node_ = next;
    return true;
  }

  // We need to traverse siblings, walking up the parent chain until
  // we find a valid sibling.
  for (next = node_;
       !next.isNull() && next != root_;
       next = next.parentNode()) {
    WebKit::WebNode sibling = next.nextSibling();
    if (!sibling.isNull()) {
      node_ = sibling;
      return true;
    }
  }

  return false;
}

}  // namespace

namespace chromium {

DomDocument* CreateDocument(const WebKit::WebDocument& document) {
  return new ChromiumDocument(document);
}

}  // namespace chromium

}  // namespace pagespeed
