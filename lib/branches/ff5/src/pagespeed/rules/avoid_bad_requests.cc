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

#include "pagespeed/rules/avoid_bad_requests.h"

#include "base/logging.h"
#include "pagespeed/core/formatter.h"
#include "pagespeed/core/pagespeed_input.h"
#include "pagespeed/core/resource.h"
#include "pagespeed/core/result_provider.h"
#include "pagespeed/core/rule_input.h"
#include "pagespeed/l10n/l10n.h"
#include "pagespeed/proto/pagespeed_output.pb.h"

namespace pagespeed {

namespace rules {

AvoidBadRequests::AvoidBadRequests()
    : pagespeed::Rule(pagespeed::InputCapabilities()) {
}

const char* AvoidBadRequests::name() const {
  return "AvoidBadRequests";
}

UserFacingString AvoidBadRequests::header() const {
  // TRANSLATOR: The name of a Page Speed rule that tells users to try and avoid
  // writing pages that generate bad HTTP requests (i.e. requests that return
  // HTTP 404 or HTTP 410 codes), for example by referencing a missing image or
  // style sheet.  This is displayed in a list of rule names that Page Speed
  // generates, telling webmasters which rules they broke in their website.
  return _("Avoid bad requests");
}

bool AvoidBadRequests::AppendResults(const RuleInput& rule_input,
                                     ResultProvider* provider) {
  const PagespeedInput& input = rule_input.pagespeed_input();
  const Resource* primary_resource =
      input.GetResourceWithUrlOrNull(input.primary_resource_url());
  for (int i = 0, num = input.num_resources(); i < num; ++i) {
    const Resource& resource = input.GetResource(i);
    if (&resource == primary_resource) {
      // Don't include the primary resource in the analysis.
      continue;
    }
    const int status_code = resource.GetResponseStatusCode();
    if (status_code == 404 || status_code == 410) {
      // TODO(mdsteele) It would be better if we could store the actual status
      // code in the Result object, so that the formatter could report it to
      // the user.
      Result* result = provider->NewResult();
      Savings* savings = result->mutable_savings();
      savings->set_requests_saved(1);

      result->add_resource_urls(resource.GetRequestUrl());
    }
  }
  return true;
}

void AvoidBadRequests::FormatResults(const ResultVector& results,
                                     RuleFormatter* formatter) {
  if (results.empty()) {
    return;
  }

  UrlBlockFormatter* body = formatter->AddUrlBlock(
      // TRANSLATOR: Header at the top of a list of URLs that Page Speed
      // detected as bad requests (requesting them returned HTTP codes 404 or
      // 410).  It describes the problem to the user, and tells them how to fix
      // it by eliminating the bad requests.
      _("The following requests are returning 404/410 responses.  Either fix "
        "the broken links, or remove the references to the non-existent "
        "resources."));

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
