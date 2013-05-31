// Copyright 2012 Google Inc. All Rights Reserved.
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

#include "pagespeed/rules/avoid_landing_page_redirects.h"

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
#include "pagespeed/core/resource_cache_computer.h"
#include "pagespeed/core/resource_util.h"
#include "pagespeed/core/result_provider.h"
#include "pagespeed/core/rule_input.h"
#include "pagespeed/core/string_util.h"
#include "pagespeed/core/uri_util.h"
#include "pagespeed/l10n/l10n.h"
#include "pagespeed/proto/pagespeed_output.pb.h"

namespace {

const char* kRuleName = "AvoidLandingPageRedirects";
const char* kLoginSubstring = "login";

const int64 kMillisInADay = 1000 * 60 * 60 * 24;
const int64 kMillisInAWeek = kMillisInADay * 7;

const pagespeed::RedirectionDetails* GetDetails(
    const pagespeed::Result& result) {
  const pagespeed::ResultDetails& details = result.details();
  if (!details.HasExtension(
          pagespeed::RedirectionDetails::message_set_extension)) {
    LOG(DFATAL) << "RedirectionDetails missing.";
    return NULL;
  }

  return &details.GetExtension(
      pagespeed::RedirectionDetails::message_set_extension);
}


bool SortRuleResultsByRedirection(const pagespeed::Result* lhs,
                                  const pagespeed::Result* rhs) {
  const pagespeed::RedirectionDetails* lhs_details = GetDetails(*lhs);
  const pagespeed::RedirectionDetails* rhs_details = GetDetails(*rhs);
  return lhs_details->chain_index() < rhs_details->chain_index();
}
}  // namespace

namespace pagespeed {

namespace rules {

AvoidLandingPageRedirects::AvoidLandingPageRedirects()
    : Rule(InputCapabilities()) {
}

const char* AvoidLandingPageRedirects::name() const {
  return kRuleName;
}

UserFacingString AvoidLandingPageRedirects::header() const {
  // TRANSLATOR: The name of a Page Speed rule that tells users to avoid
  // redirects at the landing page. The landing page is the root
  // HTML document that was requested the user in the browser's address bar.
  // This is displayed in a list of rule names that Page Speed generates.
  return _("Avoid landing page redirects");
}

bool AvoidLandingPageRedirects::AppendResults(
    const RuleInput& rule_input, ResultProvider* provider) {
  const PagespeedInput& input = rule_input.pagespeed_input();
  const Resource* primary_resource =
      input.GetResourceCollection().GetPrimaryResourceOrNull();

  if (primary_resource == NULL) {
    LOG(ERROR) << "Cannot find primary resource.";
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
    // bad redirect.
    return true;
  }

  // All redirections should be avoided for landing page. We flag both temporary
  // and permanent redirections.
  for (int idx = 0, size = chain->size(); idx < size; ++idx) {
    const Resource* resource =  chain->at(idx);
    if (resource->GetResourceType() != REDIRECT) {
      // The last resource in each chain is the final resource, which
      // should not be considered here.
      continue;
    }
    // We want to record the redirect and its destination so we can present that
    // information in the UI.
    if (idx == size - 1) {
      continue;  // This is the last redirection.
    }

    const std::string& url = resource->GetRequestUrl();
    GURL gurl(url);
    const std::string& next_url = chain->at(idx+1)->GetRequestUrl();
    GURL next_gurl(next_url);

    Result* result = provider->NewResult();
    result->add_resource_urls(url);
    result->add_resource_urls(next_url);
    Savings* savings = result->mutable_savings();
    savings->set_requests_saved(1);

    ResultDetails* details = result->mutable_details();
    RedirectionDetails* redirection_details =
        details->MutableExtension(
            RedirectionDetails::message_set_extension);

    bool permanent_redirection = (resource->GetResponseStatusCode() == 301);

    bool cacheable = permanent_redirection;
    int64 freshness_lifetime_millis = 0;
    ResourceCacheComputer comp(resource);
    if (comp.GetFreshnessLifetimeMillis(&freshness_lifetime_millis)) {
      cacheable = freshness_lifetime_millis >= kMillisInAWeek;
      redirection_details->set_freshness_lifetime_millis(
          freshness_lifetime_millis);
      LOG(INFO) << "freshness_lifetime_millis: " << freshness_lifetime_millis;
      // An explicit cache freshness life time is specified, the redirection is
      // not permanent by any way..
      redirection_details->set_is_permanent(false);
    } else {
      redirection_details->set_is_permanent(permanent_redirection);
    }
    redirection_details->set_is_cacheable(cacheable);
    bool same_host = (gurl.host() == next_gurl.host());
    redirection_details->set_is_same_host(same_host);

    const std::string login(kLoginSubstring);
    std::string::const_iterator login_it =
    std::search(next_url.begin(), next_url.end(),
                login.begin(), login.end(),
                string_util::CaseInsensitiveCompareASCII());
    bool is_login = (login_it != next_url.end());
    redirection_details->set_is_likely_login(is_login);

    bool is_callback = (next_gurl.query().find(url) != std::string::npos);
    redirection_details->set_is_likely_callback(is_callback);

    redirection_details->set_chain_index(idx);
    redirection_details->set_chain_length(size);
  }

  return true;
}

void AvoidLandingPageRedirects::FormatResults(
    const ResultVector& results, RuleFormatter* formatter) {
  UrlBlockFormatter* body = formatter->AddUrlBlock(
      // TRANSLATOR: Header at the top of a list of URLs that Page Speed
      // detected as a chain of HTTP redirections. It tells the user to fix
      // the problem by removing the URLs that redirect to others.
      _("To speed up page load times for visitors of your site, remove as many "
        "landing page redirections as possible, and make any required "
        "redirections cacheable if possible."));

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

    const RedirectionDetails* details = GetDetails(result);
    if (details == NULL) {
      body->AddUrlResult(
          // TRANSLATOR: Message displayed to indicate that a URL
          // redirects to another URL, e.g "http://example.com/ is a redirect to
          // http://www.example.com/".
          _("$1 is a redirect to $2"),
          UrlArgument(result.resource_urls(0)),
          UrlArgument(result.resource_urls(1)));
      continue;
    }

    if (!details->is_cacheable()) {
      // Not long cacheable.
      if (details->has_freshness_lifetime_millis() &&
          details->freshness_lifetime_millis() > 0) {
        body->AddUrlResult(
            // TRANSLATOR: Message displayed to indicate that a URL
            // redirects to another URL, and the redirection is not cacheable.
            _("$1 is a short-cacheable ($3) redirect to $2"),
            UrlArgument(result.resource_urls(0)),
            UrlArgument(result.resource_urls(1)),
            DurationArgument(details->freshness_lifetime_millis()));
      } else {
        body->AddUrlResult(
            // TRANSLATOR: Message displayed to indicate that a URL
            // redirects to another URL, and the redirection is not cacheable.
            _("$1 is a non-cacheable redirect to $2"),
            UrlArgument(result.resource_urls(0)),
            UrlArgument(result.resource_urls(1)));
      }
    } else if (!details->is_permanent()) {
      // Cacheable long enough, but not permanent.
      if (details->has_freshness_lifetime_millis()) {
        body->AddUrlResult(
            // TRANSLATOR: Message displayed to indicate that a URL
            // redirects to another URL, and the redirection is not cacheable.
            _("$1 is a cacheable ($3) redirect to $2"),
            UrlArgument(result.resource_urls(0)),
            UrlArgument(result.resource_urls(1)),
            DurationArgument(details->freshness_lifetime_millis()));
      } else {
        body->AddUrlResult(
            // TRANSLATOR: Message displayed to indicate that a URL
            // redirects to another URL, and the redirection is cacheable.
            _("$1 is a cacheable redirect to $2"),
            UrlArgument(result.resource_urls(0)),
            UrlArgument(result.resource_urls(1)));
      }
    } else {
      body->AddUrlResult(
          // TRANSLATOR: Message displayed to indicate that a URL
          // redirects to another URL, and the redirection is permanent.
          _("$1 is a permanent redirect to $2"),
          UrlArgument(result.resource_urls(0)),
          UrlArgument(result.resource_urls(1)));
    }
  }
}

void AvoidLandingPageRedirects::SortResultsInPresentationOrder(
    ResultVector* rule_results) const {
  // Sort the results in request order so that the user can easily see the
  // redirection chain.
  std::stable_sort(rule_results->begin(),
                   rule_results->end(),
                   SortRuleResultsByRedirection);
}

}  // namespace rules

}  // namespace pagespeed
