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

#include "pagespeed/rules/minimize_redirects.h"

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

const char* kRuleName = "MinimizeRedirects";

}  // namespace

namespace pagespeed {

namespace rules {

MinimizeRedirects::MinimizeRedirects()
    : pagespeed::Rule(pagespeed::InputCapabilities()) {
}

const char* MinimizeRedirects::name() const {
  return kRuleName;
}

UserFacingString MinimizeRedirects::header() const {
  // TRANSLATOR: The name of a Page Speed rule that tells users to minimize
  // HTTP redirects from one URL to another URL. This is displayed in a list of
  // rule names that Page Speed generates.
  return _("Minimize redirects");
}

/**
 * Gather redirects to compute the redirect graph, then traverse the
 * redirect graph and append a result for each redirect sequence
 * found.
 * In the case of redirect loops, traversal stops when trying to
 * process an URL that has already been visited.
 *
 * Examples:
 *   Redirect chain:
 *     input:  a -> b, b -> c
 *     output: a, b, c
 *
 *   Redirect loop:
 *     input:  a -> b, b -> c, c -> a
 *     output: a, b, c, a
 *
 *   Redirect diamond:
 *     input:  a -> [b, c], b -> d, c -> d
 *     output: a, b, d, c, d
 */
bool MinimizeRedirects::AppendResults(const RuleInput& rule_input,
                                      ResultProvider* provider) {
  const PagespeedInput& input = rule_input.pagespeed_input();
  const std::string& primary_resource_url_fragment =
      input.primary_resource_url();
  std::string primary_resource_url;
  if (!uri_util::GetUriWithoutFragment(primary_resource_url_fragment,
                                       &primary_resource_url)) {
    primary_resource_url = primary_resource_url_fragment;
  }
  const Resource* primary_resource =
      input.GetResourceWithUrlOrNull(primary_resource_url);

  const RedirectRegistry::RedirectChain* landing_chain = NULL;
  if (primary_resource != NULL) {
    landing_chain = input.GetResourceCollection()
        .GetRedirectRegistry()->GetRedirectChainOrNull(primary_resource);
  }

  const RedirectRegistry::RedirectChainVector& redirect_chains =
      input.GetResourceCollection().GetRedirectRegistry()->GetRedirectChains();

  for (RedirectRegistry::RedirectChainVector::const_iterator it =
       redirect_chains.begin(), end = redirect_chains.end();
       it != end;
       ++it) {
    const RedirectRegistry::RedirectChain& chain = *it;
    if (chain.size() <= 1) {
      // This can happen if the destination URL of a redirect doesn't
      // have an associated Resource in the PagespeedInput.
      LOG(INFO) << "Skipping redirect chain with one resource.";
      continue;
    }

    // Skip the landing page redirect chain. This chain is handled in the
    // MakeLandingPageRedirectsCacheable rule.
    if (&chain == landing_chain) {
      continue;
    }
    Result* result = provider->NewResult();
    for (RedirectRegistry::RedirectChain::const_iterator cit = chain.begin(),
        cend = chain.end();
        cit != cend;
        ++cit) {
      result->add_resource_urls((*cit)->GetRequestUrl());
    }

    pagespeed::Savings* savings = result->mutable_savings();
    savings->set_requests_saved(result->resource_urls_size() - 1);
  }

  return true;
}

void MinimizeRedirects::FormatResults(const ResultVector& results,
                                      RuleFormatter* formatter) {
  for (ResultVector::const_iterator iter = results.begin(),
           end = results.end();
       iter != end;
       ++iter) {
    UrlBlockFormatter* body = formatter->AddUrlBlock(
        // TRANSLATOR: Header at the top of a list of URLs that Page Speed
        // detected as a chain of HTTP redirections. It tells the user to fix
        // the problem by removing the URLs that redirect to others.
        _("Remove the following redirect chain if possible:"));

    const Result& result = **iter;
    for (int url_idx = 0; url_idx < result.resource_urls_size(); url_idx++) {
      body->AddUrl(result.resource_urls(url_idx));
    }
  }
}

}  // namespace rules

}  // namespace pagespeed
