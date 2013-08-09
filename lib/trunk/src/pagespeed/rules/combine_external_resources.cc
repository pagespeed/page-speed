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

#include "pagespeed/rules/combine_external_resources.h"

#include <string>

#include "base/logging.h"
#include "googleurl/src/gurl.h"
#include "pagespeed/core/formatter.h"
#include "pagespeed/core/pagespeed_input.h"
#include "pagespeed/core/resource.h"
#include "pagespeed/core/result_provider.h"
#include "pagespeed/core/rule_input.h"
#include "pagespeed/l10n/l10n.h"
#include "pagespeed/proto/pagespeed_output.pb.h"

namespace {

// Allow 2 or fewer resources. Many sites have a site-wide CSS/JS file
// as well as a per-page CSS/JS file. Thus we allow 2 resources per
// hostname without showing a warning.
const size_t kMaxResourcesPerDomain = 2;

}  // namespace

namespace pagespeed {

namespace rules {

CombineExternalResources::CombineExternalResources(ResourceType resource_type)
    : pagespeed::Rule(pagespeed::InputCapabilities(
        pagespeed::InputCapabilities::ONLOAD |
        pagespeed::InputCapabilities::REQUEST_START_TIMES)),
      resource_type_(resource_type) {
}

bool CombineExternalResources::AppendResults(const RuleInput& rule_input,
                                             ResultProvider* provider) {
  const PagespeedInput& input = rule_input.pagespeed_input();
  const HostResourceMap& host_resource_map = *input.GetHostResourceMap();

  for (HostResourceMap::const_iterator iter = host_resource_map.begin(),
           end = host_resource_map.end();
       iter != end;
       ++iter) {
    ResourceSet violations;
    for (ResourceSet::const_iterator resource_iter = iter->second.begin(),
             resource_end = iter->second.end();
         resource_iter != resource_end;
         ++resource_iter) {
      const pagespeed::Resource* resource = *resource_iter;

      // exclude non-http resources
      std::string protocol = resource->GetProtocol();
      if (protocol != "http" && protocol != "https") {
        continue;
      }

      if (resource->GetResourceType() != resource_type_) {
        continue;
      }

      // exclude post-onload resources
      if (input.IsResourceLoadedAfterOnload(*resource)) {
        continue;
      }

      const std::string& host = iter->first;
      if (host.empty()) {
        LOG(DFATAL) << "Empty host while processing "
                    << resource->GetRequestUrl();
      }

      violations.insert(resource);
    }

    if (violations.size() > kMaxResourcesPerDomain) {
      Result* result = provider->NewResult();
      int requests_saved = violations.size() - kMaxResourcesPerDomain;
      for (ResourceSet::const_iterator violation_iter = violations.begin(),
               violation_end = violations.end();
           violation_iter != violation_end;
           ++violation_iter) {
        result->add_resource_urls((*violation_iter)->GetRequestUrl());
      }

      pagespeed::Savings* savings = result->mutable_savings();
      savings->set_requests_saved(requests_saved);
    }
  }

  return true;
}

void CombineExternalResources::FormatResults(const ResultVector& results,
                                             RuleFormatter* formatter) {
  UserFacingString body_tmpl;
  if (resource_type_ == CSS) {
    // TRANSLATOR: Descriptive header describing a list of CSS resources that
    // are all served from a single domain (in violation of the
    // CombineExternalResources rule).  It says how many resources were loaded
    // from that domain, gives the domain name itself, and is followed by a list
    // of the URLs of those resources.  It then tells the webmaster how to solve
    // the problem, by combining the resources into fewer files.
    body_tmpl = _("There are %(NUM_FILES)s CSS files served from "
                  "%(DOMAIN_NAME)s. They should be combined into as few files "
                  "as possible.");
  } else if (resource_type_ == JS) {
    // TRANSLATOR: Descriptive header describing a list of JavaScript resources
    // that are all served from a single domain (in violation of the
    // CombineExternalResources rule).  It says how many resources were loaded
    // from that domain, gives the domain name itself, and is followed by a list
    // of the URLs of those resources.  It then tells the webmaster how to solve
    // the problem, by combining the resources into fewer files.
    body_tmpl = _("There are %(NUM_FILES)s JavaScript files served from "
                  "%(DOMAIN_NAME)s. They should be combined into as few files "
                  "as possible.");
  } else {
    LOG(DFATAL) << "Unknown violation type " << resource_type_;
    return;
  }

  for (ResultVector::const_iterator iter = results.begin(),
           end = results.end();
       iter != end;
       ++iter) {
    const Result& result = **iter;

    GURL url(result.resource_urls(0));
    UrlBlockFormatter* body = formatter->AddUrlBlock(
        body_tmpl, IntArgument("NUM_FILES", result.resource_urls_size()),
        StringArgument("DOMAIN_NAME", url.host()));

    for (int idx = 0; idx < result.resource_urls_size(); idx++) {
      body->AddUrl(result.resource_urls(idx));
    }
  }
}

CombineExternalJavaScript::CombineExternalJavaScript()
    : CombineExternalResources(JS) {
}

const char* CombineExternalJavaScript::name() const {
  return "CombineExternalJavaScript";
}

UserFacingString CombineExternalJavaScript::header() const {
  // TRANSLATOR: The name of a Page Speed rule that tells webmasters to combine
  // external JavaScript resources that are loaded from the same domain.  This
  // appears in a list of rule names generated by Page Speed, telling webmasters
  // which rules they broke in their website.
  return _("Combine external JavaScript");
}

CombineExternalCss::CombineExternalCss()
    : CombineExternalResources(CSS) {
}

const char* CombineExternalCss::name() const {
  return "CombineExternalCss";
}

UserFacingString CombineExternalCss::header() const {
  // TRANSLATOR: The name of a Page Speed rule that tells webmasters to combine
  // external CSS resources that are loaded from the same domain.  This
  // appears in a list of rule names generated by Page Speed, telling webmasters
  // which rules they broke in their website.
  return _("Combine external CSS");
}

}  // namespace rules

}  // namespace pagespeed
