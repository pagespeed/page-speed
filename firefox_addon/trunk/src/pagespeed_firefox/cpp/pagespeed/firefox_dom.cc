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

#include "inIDOMUtils.h"
#include "nsIDOM3Node.h"
#include "nsIDOMAttr.h"
#include "nsIDOMCSSStyleDeclaration.h"
#include "nsIDOMCSSStyleRule.h"
#include "nsIDOMDocument.h"
#include "nsIDOMDocumentTraversal.h"
#include "nsIDOMElement.h"
#include "nsIDOMElementCSSInlineStyle.h"
#include "nsIDOMHTMLDocument.h"
#include "nsIDOMHTMLIFrameElement.h"
#include "nsIDOMHTMLImageElement.h"
#include "nsIDOMNamedNodeMap.h"
#include "nsIDOMNodeFilter.h"
#include "nsIDOMNSHTMLImageElement.h"
#include "nsIDOMTreeWalker.h"
#include "nsISupportsArray.h"
#include "nsIURI.h"
#include "nsNetCID.h"
#include "nsNetUtil.h"
#include "nsServiceManagerUtils.h"  // for do_GetService
#include "nsStringAPI.h"

#include "base/logging.h"

namespace {

const char* kDomUtilsContractId = "@mozilla.org/inspector/dom-utils;1";

bool GetStylePropertyByName(nsIDOMCSSStyleDeclaration* style,
                            const std::string& name,
                            std::string* property_value) {
  NS_ConvertASCIItoUTF16 ns_name(name.c_str());

  nsString value;
  nsresult rv = style->GetPropertyValue(ns_name, value);
  if (NS_FAILED(rv) || value.Length() == 0) {
    return false;
  }

  NS_ConvertUTF16toUTF8 converter(value);
  *property_value = converter.get();
  return true;
}

bool GetInlineStylePropertyByName(nsIDOMElement* element,
                                  const std::string& name,
                                  std::string* property_value) {
  nsresult rv;
  nsCOMPtr<nsIDOMElementCSSInlineStyle> inline_style(
      do_QueryInterface(element, &rv));
  if (NS_FAILED(rv) || !inline_style) {
    return false;
  }

  nsCOMPtr<nsIDOMCSSStyleDeclaration> style;
  rv = inline_style->GetStyle(getter_AddRefs(style));
  if (NS_FAILED(rv) || !style) {
    return false;
  }

  return GetStylePropertyByName(style, name, property_value);
}

bool GetCascadedStylePropertyByName(nsIDOMElement* element,
                                    const std::string& name,
                                    std::string* property_value) {
  nsresult rv;
  nsCOMPtr<inIDOMUtils> dom_utils(do_GetService(kDomUtilsContractId, &rv));
  if (NS_FAILED(rv) || !dom_utils) {
    return false;
  }

  nsCOMPtr<nsISupportsArray> style_rules;
  rv = dom_utils->GetCSSStyleRules(element, getter_AddRefs(style_rules));
  if (NS_FAILED(rv) || !style_rules) {
    return false;
  }

  PRUint32 num_style_rules;
  rv = style_rules->Count(&num_style_rules);
  if (NS_FAILED(rv) || num_style_rules == 0) {
    return false;
  }

  for (int i = 0; i < num_style_rules; i++) {
    nsCOMPtr<nsISupports> rule_supports = style_rules->ElementAt(i);
    nsCOMPtr<nsIDOMCSSStyleRule> rule(do_QueryInterface(rule_supports, &rv));
    if (NS_FAILED(rv) || !rule) {
      continue;
    }

    nsCOMPtr<nsIDOMCSSStyleDeclaration> style;
    rv = rule->GetStyle(getter_AddRefs(style));
    if (NS_FAILED(rv) || !style) {
      continue;
    }

    if (GetStylePropertyByName(style, name, property_value)) {
      return true;
    }
  }

  return false;
}

class NodeFilter : public nsIDOMNodeFilter {
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

std::string FirefoxDocument::GetDocumentUrl() const {
  nsresult rv;
  nsCOMPtr<nsIDOMHTMLDocument> html_document(
      do_QueryInterface(document_, &rv));
  if (!NS_FAILED(rv)) {
    nsString url;
    rv = html_document->GetURL(url);
    if (!NS_FAILED(rv)) {
      NS_ConvertUTF16toUTF8 converter(url);
      return converter.get();
    } else {
      LOG(ERROR) << "GetURL failed.";
      return "";
    }
  } else {
    LOG(ERROR) << "nsIDOMHTMLDocument query-interface failed.";
    return "";
  }
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

bool FirefoxElement::GetResourceUrl(std::string* url) const {
  std::string src;
  if (!GetAttributeByName("src", &src)) {
    return false;
  }

  nsCOMPtr<nsIDOM3Node> node = do_QueryInterface(element_);
  if (!node) {
    return false;
  }

  nsString base_uri_str;
  nsresult rv = node->GetBaseURI(base_uri_str);
  if (NS_FAILED(rv) || base_uri_str.Length() == 0) {
    return false;
  }

  // Convert from an nsString to an nsIURI.
  nsCOMPtr<nsIURI> base_uri;
  rv = NS_NewURI(getter_AddRefs(base_uri),
                 base_uri_str,
                 nsnull,
                 nsnull);
  if (NS_FAILED(rv) || !base_uri) {
    return false;
  }

  // Convert the (possibly) relative src to an absolute URL, by
  // resolving it relative to base_uri.
  nsCString spec(src.c_str(), src.size());
  nsCOMPtr<nsIURI> uri;
  rv = NS_NewURI(getter_AddRefs(uri),
                 spec,
                 nsnull,
                 base_uri);
  if (NS_FAILED(rv) || !uri) {
    return false;
  }

  nsCString uri_spec;
  uri->GetSpec(uri_spec);
  *url = uri_spec.get();
  return true;
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

bool FirefoxElement::GetIntPropertyByName(const std::string& name,
                                          int* property_value) const {
  if (name == "clientWidth" || name == "clientHeight") {
    nsresult rv;
    nsCOMPtr<nsIDOMHTMLImageElement> image(do_QueryInterface(element_, &rv));
    if (!NS_FAILED(rv)) {
      if (name == "clientWidth") {
        image->GetWidth(property_value);
      } else {
        image->GetHeight(property_value);
      }
      return true;
    }
  } else if (name == "naturalWidth" || name == "naturalHeight") {
    nsresult rv;
    nsCOMPtr<nsIDOMNSHTMLImageElement> image(do_QueryInterface(element_, &rv));
    if (!NS_FAILED(rv)) {
      if (name == "naturalWidth") {
        image->GetNaturalWidth(property_value);
      } else {
        image->GetNaturalHeight(property_value);
      }
      return true;
    }
  }

  return false;
}

bool FirefoxElement::GetCSSPropertyByName(const std::string& name,
                                          std::string* property_value) const {
  // First check inline styles, since they take precedence.
  if (GetInlineStylePropertyByName(element_, name, property_value)) {
    return true;
  }

  // Next check cascaded properties (e.g. those in a style block or
  // external stylesheet.
  return GetCascadedStylePropertyByName(element_, name, property_value);
}

}  // namespace pagespeed
