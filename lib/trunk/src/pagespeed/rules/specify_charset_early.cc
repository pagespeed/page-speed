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

#include "pagespeed/rules/specify_charset_early.h"

#include <algorithm>  // for search

#include "base/logging.h"
#include "base/string_util.h"
#include "pagespeed/core/formatter.h"
#include "pagespeed/core/pagespeed_input.h"
#include "pagespeed/core/resource.h"
#include "pagespeed/core/resource_util.h"
#include "pagespeed/core/result_provider.h"
#include "pagespeed/html/html_tag.h"
#include "pagespeed/proto/pagespeed_output.pb.h"

namespace {

const size_t kLateThresholdBytes = 1024;

bool HasCharsetInContentTypeHeader(const std::string& header) {
  pagespeed::resource_util::DirectiveMap directives;
  if (!pagespeed::resource_util::GetHeaderDirectives(header, &directives)) {
    return false;
  }

  if (directives.find("charset") == directives.end()) {
    return false;
  }

  return !directives["charset"].empty();
}

// Used to do a case-insensitive search.
bool CaseInsensitiveBinaryPredicate(char x, char y) {
  return ToLowerASCII(x) == ToLowerASCII(y);
}

bool HasCharsetInMetaTag(const std::string& body, size_t max_bytes_to_scan) {
  const std::string meta_tag = "<meta";
  std::string::const_iterator last_byte_to_scan =
      body.begin() + max_bytes_to_scan;
  std::string::const_iterator start_offset = body.begin();
  while (start_offset < last_byte_to_scan) {
    std::string::const_iterator meta_tag_it = std::search(
        start_offset, last_byte_to_scan,
        meta_tag.begin(), meta_tag.end(),
        CaseInsensitiveBinaryPredicate);
    if (meta_tag_it == last_byte_to_scan) {
      // We've scanned the first max_bytes_to_scan bytes, so we're done.
      return false;
    }
    start_offset = meta_tag_it + meta_tag.size();

    // Check to see if there is a charset in the meta tag.
    pagespeed::html::HtmlTag html_tag;
    const char* meta_tag = html_tag.ReadTag(
        // TODO: is this &* really necessary to switch from
        // std::string::const_iterator to const char*?
        &*meta_tag_it,
        body.data() + body.size());
    if (meta_tag == NULL) {
      // Not a valid meta tag, so skip it.
      continue;
    }

    if (!html_tag.HasAttrValue("http-equiv")) {
      continue;
    }

    if (!LowerCaseEqualsASCII(
            html_tag.GetAttrValue("http-equiv"), "content-type")) {
      continue;
    }

    if (!html_tag.HasAttrValue("content")) {
      continue;
    }

    if (HasCharsetInContentTypeHeader(html_tag.GetAttrValue("content"))) {
      return true;
    }
  }

  return false;
}

}  // namespace

namespace pagespeed {

namespace rules {

SpecifyCharsetEarly::SpecifyCharsetEarly() {
}

const char* SpecifyCharsetEarly::name() const {
  return "SpecifyCharsetEarly";
}

const char* SpecifyCharsetEarly::header() const {
  return "Specify a character set early";
}

const char* SpecifyCharsetEarly::documentation_url() const {
  return "rendering.html#SpecifyCharsetEarly";
}

bool SpecifyCharsetEarly::AppendResults(const PagespeedInput& input,
                                        ResultProvider* provider) {
  for (int idx = 0, num = input.num_resources(); idx < num; ++idx) {
    const Resource& resource = input.GetResource(idx);
    const ResourceType resource_type = resource.GetResourceType();
    const std::string& content_type =
        resource.GetResponseHeader("Content-Type");

    if (resource_type != HTML) {
      const bool might_be_html = resource_type == OTHER && content_type.empty();
      if (!might_be_html) {
        // This rule only applies to HTML resources. However, if the
        // Content-Type header is not specified, it might be an HTML
        // resource that's missing a Content-Type, so include it in
        // the evaluation.
        continue;
      }
    }

    if (HasCharsetInContentTypeHeader(content_type)) {
      // There is a valid charset in the Content-Type header, so don't
      // flag this resource.
      continue;
    }

    const std::string& body = resource.GetResponseBody();
    if (body.size() < kLateThresholdBytes) {
      // The response body is small, so this rule doesn't apply.
      continue;
    }

    size_t max_bytes_to_scan = body.size();
    if (body.size() > kLateThresholdBytes) {
      max_bytes_to_scan = kLateThresholdBytes;
    }
    if (HasCharsetInMetaTag(body, max_bytes_to_scan)) {
      // There is a valid charset in a <meta> tag, so don't flag this
      // resource.
      continue;
    }

    // There was no charset found in the Content-Type header or in the
    // body, so we should flag a violation.

    Result* result = provider->NewResult();

    Savings* savings = result->mutable_savings();
    savings->set_page_reflows_saved(1);

    result->add_resource_urls(resource.GetRequestUrl());
  }

  return true;
}

void SpecifyCharsetEarly::FormatResults(const ResultVector& results,
                                        Formatter* formatter) {
  if (results.empty()) {
    return;
  }

  Formatter* body = formatter->AddChild(
      "The following resources have no character set specified "
      "or have a non-default character set specified late in the "
      "document. Specifying a character set early in these "
      "documents can speed up browser rendering.");

  for (ResultVector::const_iterator iter = results.begin(),
           end = results.end();
       iter != end;
       ++iter) {
    const Result& result = **iter;
    if (result.resource_urls_size() != 1) {
      LOG(DFATAL) << "Unexpected number of resource URLs.  Expected 1, Got "
                  << result.resource_urls_size() << ".";
      continue;
    }
    Argument url(Argument::URL, result.resource_urls(0));
    body->AddChild("$1", url);
  }
}

}  // namespace rules

}  // namespace pagespeed
