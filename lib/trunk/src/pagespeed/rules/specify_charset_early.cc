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

#include <string>
#include "base/logging.h"
#include "base/string_util.h"
#include "pagespeed/core/formatter.h"
#include "pagespeed/core/pagespeed_input.h"
#include "pagespeed/core/resource.h"
#include "pagespeed/core/resource_util.h"
#include "pagespeed/core/result_provider.h"
#include "pagespeed/core/rule_input.h"
#include "pagespeed/l10n/l10n.h"
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

}  // namespace

namespace pagespeed {

namespace rules {

SpecifyCharsetEarly::SpecifyCharsetEarly()
    : pagespeed::Rule(pagespeed::InputCapabilities(
        pagespeed::InputCapabilities::RESPONSE_BODY)) {
}

const char* SpecifyCharsetEarly::name() const {
  return "SpecifyCharsetEarly";
}

UserFacingString SpecifyCharsetEarly::header() const {
  // TRANSLATOR: The name of a Page Speed rule that tells users to ensure that
  // their webpages include a declaration of the character set (e.g. UTF-8,
  // Latin-1, or some other text encoding) being used, in the HTTP header.
  // This is displayed in a list of rule names that Page Speed generates.
  return _("Specify a character set");
}

bool SpecifyCharsetEarly::AppendResults(const RuleInput& rule_input,
                                        ResultProvider* provider) {
  const PagespeedInput& input = rule_input.pagespeed_input();

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

    // There was no charset found in the Content-Type header.
    Result* result = provider->NewResult();

    Savings* savings = result->mutable_savings();
    savings->set_page_reflows_saved(1);

    result->add_resource_urls(resource.GetRequestUrl());
  }

  return true;
}

void SpecifyCharsetEarly::FormatResults(const ResultVector& results,
                                        RuleFormatter* formatter) {
  if (results.empty()) {
    return;
  }

  UrlBlockFormatter* body = formatter->AddUrlBlock(
      // TRANSLATOR: Header at the top of a list of URLs that Page Speed
      // detected as not declaring the character set (e.g. UTF-8,
      // Latin-1, or some other text encoding) being used. It describes the
      // problem to the user, and tells them how to fix it by explicitly
      // specifying the character set near the beginning of the page.
      _("The following resources have no character set specified "
        "in their HTTP headers. Specifying a character set in HTTP headers "
        "can speed up browser rendering."));

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
    body->AddUrl(result.resource_urls(0));
  }
}

}  // namespace rules

}  // namespace pagespeed
