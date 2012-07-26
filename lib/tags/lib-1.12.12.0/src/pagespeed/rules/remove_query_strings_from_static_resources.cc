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

#include "pagespeed/rules/remove_query_strings_from_static_resources.h"

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

namespace pagespeed {

namespace rules {

RemoveQueryStringsFromStaticResources::
RemoveQueryStringsFromStaticResources()
    : pagespeed::Rule(pagespeed::InputCapabilities()) {}

const char* RemoveQueryStringsFromStaticResources::name() const {
  return "RemoveQueryStringsFromStaticResources";
}

UserFacingString RemoveQueryStringsFromStaticResources::header() const {
  // TRANSLATOR: The name of a Page Speed rule that tells webmasters to remove
  // query strings from the URLs of static resources (i.e.
  // 'www.google.com/style.css?x=2), because it hurts the cachability of the
  // resource (in this case 'style.css').  This is displayed in a list of rule
  // names that Page Speed generates, telling webmasters which rules they broke
  // in their website.
  return _("Remove query strings from static resources");
}

bool RemoveQueryStringsFromStaticResources::
AppendResults(const RuleInput& rule_input, ResultProvider* provider) {
  const PagespeedInput& input = rule_input.pagespeed_input();
  for (int i = 0, num = input.num_resources(); i < num; ++i) {
    const Resource& resource = input.GetResource(i);
    if (resource.GetRequestUrl().find('?') != std::string::npos &&
        resource_util::IsLikelyStaticResource(resource) &&
        resource_util::IsProxyCacheableResource(resource)) {
      Result* result = provider->NewResult();
      result->add_resource_urls(resource.GetRequestUrl());
    }
  }
  return true;
}

void RemoveQueryStringsFromStaticResources::
FormatResults(const ResultVector& results, RuleFormatter* formatter) {
  if (results.empty()) {
    return;
  }

  UrlBlockFormatter* body = formatter->AddUrlBlock(
      // TRANSLATOR: Descriptive header at the top of a list of URLs that
      // violate the RemoveQueryStringsFromStaticResources rule by using a query
      // string in the URL of a static resource (such as
      // www.google.com/style.css?x=2).  It describes the problem to the user,
      // and tells the user how to fix it.
      _("Resources with a \"?\" in the URL are not cached by some proxy "
        "caching servers.  Remove the query string and encode the parameters "
        "into the URL for the following resources:"));

  for (ResultVector::const_iterator iter = results.begin(),
           end = results.end(); iter != end; ++iter) {
    const Result& result = **iter;
    if (result.resource_urls_size() != 1) {
      LOG(DFATAL) << "Unexpected number of resource URLs.  Expected 1, Got "
                  << result.resource_urls_size() << ".";
      continue;
    }
    body->AddUrl(result.resource_urls(0));
  }
}

int RemoveQueryStringsFromStaticResources::
ComputeScore(const InputInformation& input_info,
             const RuleResults& results) {
  const int num_static_resources = input_info.number_static_resources();
  if (num_static_resources == 0) {
    return 100;
  }
  const int num_non_violations = num_static_resources - results.results_size();
  DCHECK(num_non_violations >= 0);
  return 100 * num_non_violations / num_static_resources;
}

double RemoveQueryStringsFromStaticResources::ComputeResultImpact(
    const InputInformation& input_info, const Result& result) {
  // TODO(mdsteele): What is the impact of this rule?  It doesn't ever actually
  //   save a request.  It _might_ decrease the response time if 1) you're
  //   behind a proxy that has the relevant bug, and 2) the proxy has this
  //   resource in cache.  In all other cases, following this rule's
  //   suggestion has no impact at all.
  return 0.0;
}

}  // namespace rules

}  // namespace pagespeed
