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

#include "pagespeed/html/external_resource_filter.h"

#include <set>
#include <string>
#include "base/string_util.h"
#include "net/instaweb/htmlparse/public/empty_html_filter.h"
#include "net/instaweb/htmlparse/public/html_name.h"
#include "net/instaweb/htmlparse/public/html_parse.h"
#include "pagespeed/core/uri_util.h"

namespace pagespeed {

namespace {

void ResolveExternalResourceUrls(
    std::vector<std::string>* external_resource_urls,
    const DomDocument* document,
    const std::string& document_url) {
  // Resolve URLs relative to their document.
  for (std::vector<std::string>::iterator
           it = external_resource_urls->begin(),
           end = external_resource_urls->end();
       it != end;
       ++it) {
    std::string resolved_uri;
    if (!uri_util::ResolveUriForDocumentWithUrl(*it,
                                                document,
                                                document_url,
                                                &resolved_uri)) {
      // We failed to resolve relative to the document, so try to
      // resolve relative to the document's URL. This will be
      // correct unless the document contains a <base> tag.
      resolved_uri = uri_util::ResolveUri(*it, document_url);
    }
    *it = resolved_uri;
  }
}

}  // namespace

namespace html {

ExternalResourceFilter::ExternalResourceFilter(
    net_instaweb::HtmlParse* html_parse) {
}

ExternalResourceFilter::~ExternalResourceFilter() {}

void ExternalResourceFilter::StartDocument() {
  external_resource_urls_.clear();
}

void ExternalResourceFilter::StartElement(net_instaweb::HtmlElement* element) {
  net_instaweb::HtmlName::Keyword keyword = element->keyword();
  if (keyword == net_instaweb::HtmlName::kScript ||
      keyword == net_instaweb::HtmlName::kImg ||
      keyword == net_instaweb::HtmlName::kIframe ||
      keyword == net_instaweb::HtmlName::kFrame ||
      keyword == net_instaweb::HtmlName::kEmbed ||
      keyword == net_instaweb::HtmlName::kSource ||
      keyword == net_instaweb::HtmlName::kAudio ||
      keyword == net_instaweb::HtmlName::kVideo ||
      keyword == net_instaweb::HtmlName::kTrack) {
    const char* src = element->AttributeValue(net_instaweb::HtmlName::kSrc);
    if (src != NULL) {
      external_resource_urls_.push_back(src);
    }
    return;
  }

  if (keyword == net_instaweb::HtmlName::kLink) {
    const char* rel = element->AttributeValue(net_instaweb::HtmlName::kRel);
    if (rel == NULL || !LowerCaseEqualsASCII(rel, "stylesheet")) {
      return;
    }
    const char* href = element->AttributeValue(net_instaweb::HtmlName::kHref);
    if (href != NULL) {
      external_resource_urls_.push_back(href);
    }
    return;
  }

  // <input type="image" src="...">
  if (keyword == net_instaweb::HtmlName::kInput) {
    const char* type = element->AttributeValue(net_instaweb::HtmlName::kType);
    if (type != NULL && LowerCaseEqualsASCII(type, "image")) {
      const char* src = element->AttributeValue(net_instaweb::HtmlName::kSrc);
      if (src != NULL) {
        external_resource_urls_.push_back(src);
      }
    }
    return;
  }

  // <object data="...">
  if (keyword == net_instaweb::HtmlName::kObject) {
    for (int i = 0; i < element->attribute_size(); ++i) {
      const net_instaweb::HtmlElement::Attribute& attr = element->attribute(i);
      if (LowerCaseEqualsASCII(attr.name_str(), "data")) {
        const char* value = attr.DecodedValueOrNull();
        if (value != NULL) {
          external_resource_urls_.push_back(value);
        }
      }
    }
    return;
  }

  // <body background="...">
  if (keyword == net_instaweb::HtmlName::kBody) {
    for (int i = 0; i < element->attribute_size(); ++i) {
      const net_instaweb::HtmlElement::Attribute& attr = element->attribute(i);
      if (LowerCaseEqualsASCII(attr.name_str(), "background")) {
        const char* value = attr.DecodedValueOrNull();
        if (value != NULL) {
          external_resource_urls_.push_back(value);
        }
      }
    }
    return;
  }
}

bool ExternalResourceFilter::GetExternalResourceUrls(
    std::vector<std::string>* out,
    const DomDocument* document,
    const std::string& document_url) const {
  // We want to uniqueify the list of URLs. The easiest way to do that
  // is to copy the contents of the vector to a set, and then back to
  // a vector.
  std::vector<std::string> tmp(external_resource_urls_);
  ResolveExternalResourceUrls(
      &tmp, document, document_url);
  std::set<std::string> unique;
  for (std::vector<std::string>::const_iterator it = tmp.begin(),
           end = tmp.end(); it != end; ++it) {
    // Only include URLs for external resources (e.g. do not include
    // data: URLs).
    if (pagespeed::uri_util::IsExternalResourceUrl(*it)) {
      unique.insert(*it);
    }
  }
  out->assign(unique.begin(), unique.end());
  return !out->empty();
}

}  // namespace html

}  // namespace pagespeed
