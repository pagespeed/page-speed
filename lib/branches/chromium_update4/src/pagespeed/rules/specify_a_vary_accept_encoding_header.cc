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

#include "pagespeed/rules/specify_a_vary_accept_encoding_header.h"

#include <string>

#include "base/logging.h"
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

SpecifyAVaryAcceptEncodingHeader::SpecifyAVaryAcceptEncodingHeader()
    : pagespeed::Rule(pagespeed::InputCapabilities()) {}

const char* SpecifyAVaryAcceptEncodingHeader::name() const {
  return "SpecifyAVaryAcceptEncodingHeader";
}

UserFacingString SpecifyAVaryAcceptEncodingHeader::header() const {
  // TRANSLATOR: The name of a Page Speed rule that tells users to ensure that
  // certain resources on their webpage are served with a "Vary" HTTP header
  // whose value is set to "Accept-Encoding".  This is displayed in a list of
  // rule names that Page Speed generates.  Note that "Vary: Accept-Encoding"
  // is code and should not be translated.
  return _("Specify a Vary: Accept-Encoding header");
}

bool SpecifyAVaryAcceptEncodingHeader::
AppendResults(const RuleInput& rule_input, ResultProvider* provider) {
  const PagespeedInput& input = rule_input.pagespeed_input();
  for (int i = 0, num = input.num_resources(); i < num; ++i) {
    const Resource& resource = input.GetResource(i);
    if (!resource_util::IsLikelyStaticResource(resource)) {
      continue;  // Check only static resources.
    }
    // Complain if:
    //   1) There's no cookie in the response,
    //   2) The resource is compressible,
    //   3) The resource is proxy-cacheable, and
    //   4) Vary: accept-encoding is not already set.
    if (resource.GetResponseHeader("Set-Cookie").empty() &&
        resource_util::IsCompressibleResource(resource) &&
        resource_util::IsProxyCacheableResource(resource)) {
      const std::string& vary_header = resource.GetResponseHeader("Vary");
      resource_util::DirectiveMap directive_map;
      if (resource_util::GetHeaderDirectives(vary_header, &directive_map) &&
          !directive_map.count("accept-encoding")) {
        Result* result = provider->NewResult();
        result->add_resource_urls(resource.GetRequestUrl());
      }
    }
  }
  return true;
}

void SpecifyAVaryAcceptEncodingHeader::
FormatResults(const ResultVector& results, RuleFormatter* formatter) {
  if (results.empty()) {
    return;
  }

  UrlBlockFormatter* body = formatter->AddUrlBlock(
      // TRANSLATOR: Header at the top of a list of URLs of webpage resources
      // that Page Speed detected as having three properties: 1) they are
      // publicly cacheable (that is, they can be cached by HTTP proxies), 2)
      // they are compressible (that is, the data may be compressed during
      // transfer), and 3) the resource does not have a "Vary" HTTP header
      // whose value is set to "Accept-Encoding".  It describes the problem to
      // the user, and explains how to fix the problem by adding a Vary:
      // Accept-Encoding header to each of the listed resources.  Note that
      // "Vary: Accept-Encoding" is code and should not be translated.
      _("The following publicly cacheable, compressible resources should have "
        "a \"Vary: Accept-Encoding\" header:"));

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

int SpecifyAVaryAcceptEncodingHeader::
ComputeScore(const InputInformation& input_info, const RuleResults& results) {
  const int num_static_resources = input_info.number_static_resources();
  if (num_static_resources == 0) {
    return 100;
  }
  const int num_non_violations = num_static_resources - results.results_size();
  DCHECK(num_non_violations >= 0);
  return 100 * num_non_violations / num_static_resources;
}

double SpecifyAVaryAcceptEncodingHeader::ComputeResultImpact(
    const InputInformation& input_info, const Result& result) {
  // TODO(mdsteele): What is the impact of this rule?  It's not really a
  //   performance issue so much as a correctness issue.
  return 0.0;
}

}  // namespace rules

}  // namespace pagespeed
