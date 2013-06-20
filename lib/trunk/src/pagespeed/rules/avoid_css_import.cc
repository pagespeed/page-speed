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

#include "pagespeed/rules/avoid_css_import.h"

#include "base/logging.h"
#include "pagespeed/core/formatter.h"
#include "pagespeed/core/pagespeed_input.h"
#include "pagespeed/core/resource.h"
#include "pagespeed/core/resource_util.h"
#include "pagespeed/core/result_provider.h"
#include "pagespeed/core/rule_input.h"
#include "pagespeed/css/external_resource_finder.h"
#include "pagespeed/l10n/l10n.h"
#include "pagespeed/proto/pagespeed_output.pb.h"

namespace pagespeed {

namespace rules {

AvoidCssImport::AvoidCssImport()
    : pagespeed::Rule(pagespeed::InputCapabilities(
        pagespeed::InputCapabilities::RESPONSE_BODY)) {
}

const char* AvoidCssImport::name() const {
  return "AvoidCssImport";
}

UserFacingString AvoidCssImport::header() const {
  // TRANSLATOR: The name of a Page Speed rule telling webmasters to avoid using
  // the @import directive in their CSS style sheets ("@import" is code, and
  // should not be translated).  This is displayed in a list of rule names that
  // Page Speed generates, telling webmasters which rules they broke in their
  // website.
  return _("Avoid CSS @import");
}

bool AvoidCssImport::AppendResults(const RuleInput& rule_input,
                                   ResultProvider* provider) {
  const PagespeedInput& input = rule_input.pagespeed_input();
  for (int i = 0, num = input.num_resources(); i < num; ++i) {
    const Resource& resource = input.GetResource(i);
    if (resource.GetResourceType() != CSS) {
      continue;
    }

    std::set<std::string> external_urls;
    css::FindExternalResourcesInCssResource(resource, &external_urls);

    std::set<std::string> imported_urls;
    for (std::set<std::string>::const_iterator it = external_urls.begin(),
             end = external_urls.end(); it != end; ++it) {
      const Resource* imported_resource = input.GetResourceWithUrlOrNull(*it);
      if (imported_resource == NULL) {
        continue;
      }
      if (imported_resource->GetResourceType() == pagespeed::CSS) {
        imported_urls.insert(imported_resource->GetRequestUrl());
      }
    }
    if (imported_urls.empty()) {
      continue;
    }

    Result* result = provider->NewResult();
    result->add_resource_urls(resource.GetRequestUrl());

    Savings* savings = result->mutable_savings();

    // All @imported URLs in the same CSS document are fetched in
    // parallel, so they add one critical path length to the document
    // load.
    savings->set_critical_path_length_saved(1);

    ResultDetails* details = result->mutable_details();
    AvoidCssImportDetails* import_details =
        details->MutableExtension(
            AvoidCssImportDetails::message_set_extension);

    for (std::set<std::string>::const_iterator
             it = imported_urls.begin(), end = imported_urls.end();
         it != end;
         ++it) {
      import_details->add_imported_stylesheets(*it);
    }
  }
  return true;
}

void AvoidCssImport::FormatResults(const ResultVector& results,
                                   RuleFormatter* formatter) {
  for (ResultVector::const_iterator iter = results.begin(),
           end = results.end(); iter != end; ++iter) {
    const Result& result = **iter;
    if (result.resource_urls_size() != 1) {
      LOG(DFATAL) << "Unexpected number of resource URLs.  Expected 1, Got "
                  << result.resource_urls_size() << ".";
      continue;
    }

    const ResultDetails& details = result.details();
    if (details.HasExtension(
            AvoidCssImportDetails::message_set_extension)) {
      const AvoidCssImportDetails& import_details = details.GetExtension(
          AvoidCssImportDetails::message_set_extension);
      if (import_details.imported_stylesheets_size() > 0) {
        UrlBlockFormatter* body = formatter->AddUrlBlock(
            // TRANSLATOR: Descriptive header at the top of a list of URLs that
            // are imported by a style sheet using the @import rule ("@import"
            // is code, and should not be translated).  It gives the URL of the
            // style sheet that violates the AvoidCssImport rule (the $1
            // parameter) by using @import --- the style sheets that it imports
            // will be listed below it.  "$1" is a format token that will be
            // replaced with the URL of the style sheet that uses @import.
            _("The following external stylesheets were included in $1 "
              "using @import."), UrlArgument(result.resource_urls(0)));
        for (int i = 0, size = import_details.imported_stylesheets_size();
             i < size; ++i) {
          body->AddUrl(import_details.imported_stylesheets(i));
        }
      }
    }
  }
}

}  // namespace rules

}  // namespace pagespeed
