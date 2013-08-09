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

#include <algorithm>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "base/logging.h"
#include "googleurl/src/gurl.h"
#include "pagespeed/core/formatter.h"
#include "pagespeed/core/pagespeed_input.h"
#include "pagespeed/core/resource.h"
#include "pagespeed/core/resource_util.h"
#include "pagespeed/core/result_provider.h"
#include "pagespeed/core/rule_input.h"
#include "pagespeed/core/string_util.h"
#include "pagespeed/core/uri_util.h"
#include "pagespeed/l10n/l10n.h"
#include "pagespeed/proto/pagespeed_output.pb.h"

namespace {

const char* kRuleName = "MakeLandingPageRedirectsCacheable";
const char* kLoginSubstring = "login";

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
    LOG(ERROR) << "Primary resource URL was not set";
    return false;
  }
  const Resource* primary_resource =
      input.GetResourceWithUrlOrNull(primary_resource_url);

  if (primary_resource == NULL) {
    LOG(ERROR) << "No resource for " << primary_resource_url;
    return false;
  }
  const RedirectRegistry::RedirectChain* chain =
      input.GetResourceCollection()
      .GetRedirectRegistry()->GetRedirectChainOrNull(primary_resource);
  if (chain == NULL || chain->empty()) {
    return true;
  }

  if (resource_util::IsErrorResourceStatusCode(
          chain->back()->GetResponseStatusCode())) {
    // If the user was redirected to an error page, it should not be a
    // cached redirect.
    return true;
  }

  // We use a few heuristics to filter out common valid redirect
  // patterns: login pages, and other interstitial pages such as
  // captchas.

  // First, see if "login" is in the URL of any resources in the
  // chain. If so, skip it.
  const std::string login(kLoginSubstring);
  for (RedirectRegistry::RedirectChain::const_iterator it = chain->begin(),
           end = chain->end();
       it != end;
       ++it) {
    const Resource& r = **it;
    const std::string& url = r.GetRequestUrl();
    std::string::const_iterator login_it =
        std::search(url.begin(), url.end(),
                    login.begin(), login.end(),
                    pagespeed::string_util::CaseInsensitiveCompareASCII());
    if (login_it != url.end()) {
      // Looks like a login page. Don't flag it.
      return true;
    }
  }

  // Next, check to see if the URL of the previous resource appears in
  // the query string of the next URL. If so, skip it.
  std::string last_resource_url;
  for (RedirectRegistry::RedirectChain::const_iterator it = chain->begin(),
           end = chain->end();
       it != end;
       ++it) {
    const Resource& r = **it;
    if (!last_resource_url.empty()) {
      GURL gurl(r.GetRequestUrl());
      if (gurl.query().find(last_resource_url) != std::string::npos) {
        return true;
      }
    }
    last_resource_url = r.GetRequestUrl();
  }

  for (pagespeed::RedirectRegistry::RedirectChain::const_iterator it =
           chain->begin(), end = chain->end(); it != end; ++it) {
    const pagespeed::Resource* resource =  *it;
    if (resource->GetResourceType() != pagespeed::REDIRECT) {
      // The last resource in each chain is the final resource, which
      // should not be considered here.
      continue;
    }
    if (resource->GetResponseStatusCode() != 301 &&
        !pagespeed::resource_util::HasExplicitFreshnessLifetime(*resource)) {
      // We want to record the redirect and its destination so we can
      // present that information in the UI.
      pagespeed::RedirectRegistry::RedirectChain::const_iterator next = it;
      ++next;
      if (next == chain->end()) {
        continue;
      }

      Result* result = provider->NewResult();
      result->add_resource_urls(resource->GetRequestUrl());
      result->add_resource_urls((*next)->GetRequestUrl());
      pagespeed::Savings* savings = result->mutable_savings();
      savings->set_requests_saved(1);
    }
  }

  return true;
}

void MakeLandingPageRedirectsCacheable::FormatResults(
    const ResultVector& results, RuleFormatter* formatter) {
  UrlBlockFormatter* body = formatter->AddUrlBlock(
      // TRANSLATOR: Header at the top of a list of URLs that Page Speed
      // detected as a chain of HTTP redirections. It tells the user to fix
      // the problem by removing the URLs that redirect to others.
      _("The following landing page redirects are not cacheable. Make them "
        "cacheable to speed up page load times for repeat visitors to your "
        "site."));

  for (ResultVector::const_iterator iter = results.begin(),
           end = results.end();
       iter != end;
       ++iter) {
    const Result& result = **iter;
    if (result.resource_urls_size() != 2) {
      LOG(DFATAL) << "Unexpected number of resource URLs.  Expected 2, Got "
                  << result.resource_urls_size() << ".";
      continue;
    }

    body->AddUrlResult(
        // TRANSLATOR: Message displayed to indicate that one URL redirects to
        // another URL, e.g "http://example.com/ is an uncacheable redirect to
        // http://www.example.com/".
        _("%(ORIGINAL_URL)s is an uncacheable redirect to %(TARGET_URL)s"),
        UrlArgument("ORIGINAL_URL", result.resource_urls(0)),
        UrlArgument("TARGET_URL", result.resource_urls(1)));
  }
}

}  // namespace rules

}  // namespace pagespeed
