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

#include "pagespeed_firefox/cpp/pagespeed/firefox_dom.h"

#include "nsIDOMAttr.h"
#include "nsIDOMCSSStyleDeclaration.h"
#include "nsIDOMDocumentTraversal.h"
#include "nsIDOMElementCSSInlineStyle.h"
#include "nsIDOMHTMLIFrameElement.h"
#include "nsIDOMHTMLImageElement.h"
#include "nsIDOMNamedNodeMap.h"
#include "nsIDOMNodeFilter.h"
#include "nsIDOMTreeWalker.h"
#include "nsStringAPI.h"

#include "base/logging.h"

namespace {

class NodeFilter : public nsIDOMNodeFilter
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMNODEFILTER

  NodeFilter() {}

private:
  ~NodeFilter() {}
};

NS_IMPL_ISUPPORTS1(NodeFilter, nsIDOMNodeFilter)

NS_IMETHODIMP NodeFilter::AcceptNode(nsIDOMNode* node, PRInt16* _retval) {
  if (node == NULL || _retval == NULL) {
    return NS_ERROR_NULL_POINTER;
  }

  nsresult rv;
  nsCOMPtr<nsIDOMElement> element(do_QueryInterface(node, &rv));

  if (NS_FAILED(rv)) {
    *_retval = nsIDOMNodeFilter::FILTER_SKIP;
    return NS_OK;
  }

  *_retval = nsIDOMNodeFilter::FILTER_ACCEPT;
  return NS_OK;
}

}  // namespace

namespace pagespeed {

FirefoxDocument::FirefoxDocument(nsIDOMDocument* document)
    : document_(document) {
}

void FirefoxDocument::Traverse(DomElementVisitor* visitor) const {
  nsresult rv;
  nsCOMPtr<nsIDOMDocumentTraversal> traversal(
      do_QueryInterface(document_, &rv));
  if (!NS_FAILED(rv)) {
    nsCOMPtr<nsIDOMNodeFilter> filter(new NodeFilter);
    nsCOMPtr<nsIDOMTreeWalker> tree_walker;
    rv = traversal->CreateTreeWalker(document_,
                                     nsIDOMNodeFilter::SHOW_ALL,
                                     filter,
                                     PR_FALSE,
                                     getter_AddRefs(tree_walker));
    if (!NS_FAILED(rv)) {
      int count = 0;
      nsCOMPtr<nsIDOMNode> node;
      while (!NS_FAILED(tree_walker->NextNode(getter_AddRefs(node)))) {
        if (node == NULL) {
          break;
        }

        nsresult rv;
        nsCOMPtr<nsIDOMElement> element(do_QueryInterface(node, &rv));
        if (NS_FAILED(rv)) {
          continue;
        }

        pagespeed::FirefoxElement wrapped_element(element);
        visitor->Visit(wrapped_element);
      }
    } else {
      LOG(ERROR) << "Tree Walker creation failed.";
    }
  } else {
    LOG(ERROR) << "Node Traversal creation failed.";
  }
}

FirefoxElement::FirefoxElement(nsIDOMElement* element) : element_(element) {
}

DomDocument* FirefoxElement::GetContentDocument() const {
  nsresult rv;
  nsCOMPtr<nsIDOMHTMLIFrameElement> iframe_element(
      do_QueryInterface(element_, &rv));

  if (!NS_FAILED(rv)) {
    nsCOMPtr<nsIDOMDocument> iframe_document;
    iframe_element->GetContentDocument(getter_AddRefs(iframe_document));
    return new FirefoxDocument(iframe_document);
  } else {
    return NULL;
  }
}

std::string FirefoxElement::GetTagName() const {
  nsString tagName;
  element_->GetTagName(tagName);

  NS_ConvertUTF16toUTF8 converter(tagName);
  return converter.get();
}

std::string FirefoxElement::GetSource() const {
  {
    nsresult rv;
    nsCOMPtr<nsIDOMHTMLImageElement> as_image_element(
        do_QueryInterface(element_, &rv));
    if (!NS_FAILED(rv)) {
      nsString src;
      rv = as_image_element->GetSrc(src);
      if (!NS_FAILED(rv)) {
        NS_ConvertUTF16toUTF8 converter(src);
        return converter.get();
      }
    }
  }

  return "";
}

bool FirefoxElement::GetAttributeByName(const std::string& name,
                                        std::string* attr_value) const {
  nsCOMPtr<nsIDOMNamedNodeMap> attributes;
  nsresult rv = element_->GetAttributes(getter_AddRefs(attributes));
  if (NS_FAILED(rv)) {
    return false;
  }

  if (!attributes) {
    return false;
  }

  NS_ConvertASCIItoUTF16 ns_name(name.c_str());

  nsCOMPtr<nsIDOMNode> attr_node;
  rv = attributes->GetNamedItem(ns_name, getter_AddRefs(attr_node));
  if (NS_FAILED(rv)) {
    return false;
  }

  nsCOMPtr<nsIDOMAttr> attribute(do_QueryInterface(attr_node, &rv));
  if (NS_FAILED(rv)) {
    return false;
  }

  nsString value;
  rv = attribute->GetValue(value);
  if (NS_FAILED(rv)) {
    return false;
  }

  NS_ConvertUTF16toUTF8 converter(value);
  *attr_value = converter.get();
  return true;
}

bool FirefoxElement::GetCSSPropertyByName(const std::string& name,
                                          std::string* property_value) const {
  nsresult rv;
  nsCOMPtr<nsIDOMElementCSSInlineStyle> inline_style(
      do_QueryInterface(element_, &rv));
  if (NS_FAILED(rv) || !inline_style) {
    return false;
  }

  nsCOMPtr<nsIDOMCSSStyleDeclaration> style;
  rv = inline_style->GetStyle(getter_AddRefs(style));
  if (NS_FAILED(rv) || !style) {
    return false;
  }

  NS_ConvertASCIItoUTF16 ns_name(name.c_str());

  nsString value;
  rv = style->GetPropertyValue(ns_name, value);
  if (NS_FAILED(rv)) {
    return false;
  }

  if (value.Length() == 0) {
    return false;
  }

  NS_ConvertUTF16toUTF8 converter(value);
  *property_value = converter.get();
  return true;
}

}  // namespace pagespeed
