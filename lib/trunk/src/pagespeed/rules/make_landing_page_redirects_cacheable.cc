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

#include "pagespeed/rules/make_landing_page_redirects_cacheable.h"

#include <map>
#include <set>
#include <string>
#include <vector>

#include "base/logging.h"
#include "googleurl/src/gurl.h"
#include "googleurl/src/url_canon.h"
#include "pagespeed/core/formatter.h"
#include "pagespeed/core/pagespeed_input.h"
#include "pagespeed/core/resource.h"
#include "pagespeed/core/resource_util.h"
#include "pagespeed/core/result_provider.h"
#include "pagespeed/core/rule_input.h"
#include "pagespeed/core/uri_util.h"
#include "pagespeed/l10n/l10n.h"
#include "pagespeed/proto/pagespeed_output.pb.h"

namespace {

const char* kRuleName = "MakeLandingPageRedirectsCacheable";

bool GetNonCacheableRedirectsInRedirectChain(
    const pagespeed::PagespeedInput& input,
    const pagespeed::RuleInput::RedirectChain& chain,
    pagespeed::ResourceVector* resources) {
  for (pagespeed::RuleInput::RedirectChain::const_iterator it = chain.begin(),
      end = chain.end();
      it != end;
      ++it) {
    const pagespeed::Resource* resource =  *it;
    if (resource->GetResponseStatusCode() != 301 &&
        !pagespeed::resource_util::IsCacheableResource(*resource)) {
      resources->push_back(resource);
    }
  }
  return (!resources->empty());
}

}  // namespace

namespace pagespeed {

namespace rules {

MakeLandingPageRedirectsCacheable::MakeLandingPageRedirectsCacheable()
    : pagespeed::Rule(pagespeed::InputCapabilities()) {
}

const char* MakeLandingPageRedirectsCacheable::name() const {
  return kRuleName;
}

UserFacingString MakeLandingPageRedirectsCacheable::header() const {
  // TRANSLATOR: The name of a Page Speed rule that tells users to avoid
  // non-cacheable redirects at the landing page. The landing page is the root
  // HTML document that was requested the user in the browser's address bar.
  // This is displayed in a list of rule names that Page Speed generates.
  return _("Make landing page redirects cacheable");
}

const char* MakeLandingPageRedirectsCacheable::documentation_url() const {
  return "rtt.html#AvoidRedirects";
}

bool MakeLandingPageRedirectsCacheable::AppendResults(
    const RuleInput& rule_input, ResultProvider* provider) {
  const PagespeedInput& input = rule_input.pagespeed_input();
  const std::string& primary_resource_url_fragment =
      input.primary_resource_url();
  std::string primary_resource_url;
  if (!uri_util::GetUriWithoutFragment(primary_resource_url_fragment,
                                       &primary_resource_url)) {
    primary_resource_url = primary_resource_url_fragment;
  }

  if (primary_resource_url.empty()) {
    return false;
  }
  const Resource* primary_resource =
      input.GetResourceWithUrl(primary_resource_url);

  if (primary_resource == NULL) {
    LOG(INFO) << "No resource for " << primary_resource_url;
    return false;
  }
  const RuleInput::RedirectChain* chain =
      rule_input.GetRedirectChainOrNull(primary_resource);
  if (chain == NULL) {
    return true;
  }

  ResourceVector resources;
  if (!GetNonCacheableRedirectsInRedirectChain(input, *chain, &resources)) {
    return true;
  }

  Result* result = provider->NewResult();
  for (ResourceVector::const_iterator it = resources.begin(),
      end = resources.end();
      it != end;
      ++it) {
    result->add_resource_urls((*it)->GetRequestUrl());
  }

  pagespeed::Savings* savings = result->mutable_savings();
  savings->set_requests_saved(resources.size());

  return true;
}

void MakeLandingPageRedirectsCacheable::FormatResults(
    const ResultVector& results, RuleFormatter* formatter) {
  for (ResultVector::const_iterator iter = results.begin(),
           end = results.end();
       iter != end;
       ++iter) {
    UrlBlockFormatter* body = formatter->AddUrlBlock(
        // TRANSLATOR: Header at the top of a list of URLs that Page Speed
        // detected as a chain of HTTP redirections. It tells the user to fix
        // the problem by removing the URLs that redirect to others.
        _("The following landing page redirects are not cacheable. Make them"
          "cacheable to speed up page load times for repeat visitors to your"
          "site."));

    const Result& result = **iter;
    for (int url_idx = 0; url_idx < result.resource_urls_size(); url_idx++) {
      body->AddUrl(result.resource_urls(url_idx));
    }
  }
}

}  // namespace rules

}  // namespace pagespeed
