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

#include "base/logging.h"
#include "base/string_util.h"
#include "pagespeed/core/formatter.h"
#include "pagespeed/core/pagespeed_input.h"
#include "pagespeed/core/resource.h"
#include "pagespeed/html/html_tag.h"
#include "pagespeed/proto/pagespeed_output.pb.h"

namespace {

const int kLateThresholdBytes = 1024;

}

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
  return "rtt.html#SpecifyCharsetEarly";
}

bool SpecifyCharsetEarly::AppendResults(const PagespeedInput& input,
                                     Results* results) {
  for (int idx = 0, num = input.num_resources(); idx < num; ++idx) {
    const Resource& resource = input.GetResource(idx);
    const ResourceType resource_type = resource.GetResourceType();

    const std::string& content_type =
      resource.GetResponseHeader("Content-Type");

    // Checks only HTML document for now, and assume HTML if
    // the Content-Type is missing.
    if (content_type.empty() || resource_type == HTML) {

      // Check if CharSet exists
      bool charset_exists = false;

      int separator_idx = content_type.find(";");
      if (separator_idx != std::string::npos) {
        int charset_idx = content_type.find("charset=", separator_idx);
        if (charset_idx != std::string::npos) {
          charset_exists = true;
        }
      }

      if (!charset_exists) {
        // check for charset in response body
        const std::string& body = resource.GetResponseBody();

        // if the document is small, don't bother to check charset.
        if (body.size() < kLateThresholdBytes) {
          return true;
        }

        // TODO(lsong): use more efficent method to get the META tag.
        // The html is first coverted to lowercase, so that we can
        // find the meta tag in lowercase.
        const std::string lower_case_body = StringToLowerASCII(body);

        const char* htmlEnd = lower_case_body.c_str() +
                              lower_case_body.size();

        int start_offset = 0;
        while (start_offset < lower_case_body.size()) {
          int meta_charset_idx = lower_case_body.find("<meta", start_offset);
          if (meta_charset_idx ==  std::string::npos ||
              meta_charset_idx > kLateThresholdBytes) {
            break;
          }
          start_offset = meta_charset_idx + strlen("<meta");

          // check there is charset in the meta tag
          html::HtmlTag htmlTag;
          const char* metaTag = htmlTag.ReadTag(
              lower_case_body.c_str() + meta_charset_idx,
              htmlEnd);
          if ( metaTag == NULL ) {
            break;
          }

          if (htmlTag.HasAttrValue("http-equiv") &&
              htmlTag.GetAttrValue("http-equiv") == "content-type" &&
              htmlTag.HasAttrValue("content") ) {
            const std::string& content = htmlTag.GetAttrValue("content");
            const std::string collapsed_content =
                CollapseWhitespaceASCII(content, true);
            if ((collapsed_content.find("text/html; charset=") == 0 ||
                 collapsed_content.find("text/html;charset=") == 0) &&
                collapsed_content.size() > strlen("text/html;charset=")) {
              charset_exists = true;
              break;
            }
          }
        }
      }

      if (!charset_exists) {
        Result* result = results->add_results();
        result->set_rule_name(name());

        Savings* savings = result->mutable_savings();
        savings->set_page_reflows_saved(1);

        result->add_resource_urls(resource.GetRequestUrl());
      }
    }
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
