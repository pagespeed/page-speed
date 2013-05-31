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

#include <string>
#include <vector>

#include "base/logging.h"
#include "base/memory/scoped_ptr.h"
#include "pagespeed/core/pagespeed_input.h"
#include "pagespeed/core/resource.h"
#include "pagespeed/core/result_provider.h"
#include "pagespeed/core/string_util.h"
#include "pagespeed/proto/pagespeed_output.pb.h"
#include "pagespeed/rules/make_landing_page_redirects_cacheable.h"
#include "pagespeed/testing/pagespeed_test.h"

using pagespeed::rules::MakeLandingPageRedirectsCacheable;
using pagespeed::PagespeedInput;
using pagespeed::Resource;
using pagespeed::ResourceType;
using pagespeed::Result;
using pagespeed::Results;
using pagespeed::ResultProvider;
using pagespeed::Rule;
using pagespeed::RuleResults;

namespace {

const char* KPermanentResponsePart1 =
    "<!DOCTYPE HTML PUBLIC \"-//IETF//DTD HTML 2.0//EN\">"
    "<html><head>"
    "<title>301 Moved Permanently</title>"
    "</head><body>"
    "<h1>Moved Permanently</h1>"
    "<p>The document has moved <a href=\"";

const char* KPermanentResponsePart2 = "\">here</a>.</p> </body></html> ";

class Violation {
 public:
  Violation(int _expected_request_savings,
            const std::vector<std::string>& _urls)
      : expected_request_savings(_expected_request_savings),
        urls(_urls) {
  }

  int expected_request_savings;
  std::vector<std::string> urls;
};

class MakeLandingPageRedirectsCacheableTest : public
    ::pagespeed_testing::PagespeedTest {
 protected:
  void AddResourceUrl(const std::string& url, int status_code) {
    Resource* resource = new Resource;
    resource->SetRequestUrl(url);
    resource->SetRequestMethod("GET");
    resource->SetResponseStatusCode(status_code);
    AddResource(resource);
  }

  void AddRedirect(const std::string& url,
                   int response_code,
                   const std::string& location,
                   const std::string& cache_control_header) {
    Resource* resource = new Resource;
    resource->SetRequestUrl(url);
    resource->SetRequestMethod("GET");
    resource->SetResponseStatusCode(response_code);
    if (!location.empty()) {
      resource->AddResponseHeader("Location", location);
    }
    if (!cache_control_header.empty()) {
      resource->AddResponseHeader("Cache-Control", cache_control_header);
    }
    if (response_code == 301) {
      std::string body = KPermanentResponsePart1 +
          location + KPermanentResponsePart2;
      resource->AddResponseHeader(
          "Content-Length",
          pagespeed::string_util::IntToString(body.size()));
      resource->SetResponseBody(body);
    }
    AddResource(resource);
  }
  void AddPermanentRedirect(const std::string& url,
                            const std::string& location) {
    AddRedirect(url, 301, location, "");
  }


  void AddTemporaryRedirect(const std::string& url,
                            const std::string& location) {
    AddRedirect(url, 302, location, "");
  }


  void AddCacheableTemporaryRedirect(const std::string& url,
                                     const std::string& location) {
    AddRedirect(url, 302, location, "max-age=31536000");
  }

  void CheckViolations(const std::vector<Violation>& expected_violations) {
    MakeLandingPageRedirectsCacheable rule;

    RuleResults rule_results;
    ResultProvider provider(rule, &rule_results, 0);
    pagespeed::RuleInput rule_input(*pagespeed_input());
    rule_input.Init();
    ASSERT_TRUE(rule.AppendResults(rule_input, &provider));
    ASSERT_EQ(static_cast<size_t>(rule_results.results_size()),
              expected_violations.size());
    for (int idx = 0; idx < rule_results.results_size(); idx++) {
      const Result* result = &rule_results.results(idx);
      const Violation& violation = expected_violations[idx];

      ASSERT_EQ(violation.expected_request_savings,
                result->savings().requests_saved());

      ASSERT_EQ(violation.urls.size(),
                static_cast<size_t>(result->resource_urls_size()));

      for (int url_idx = 0;
           url_idx < result->resource_urls_size();
           ++url_idx) {
        EXPECT_EQ(violation.urls[url_idx],
                  result->resource_urls(url_idx))
            << "At index: " << url_idx;
      }
    }
  }
};

TEST_F(MakeLandingPageRedirectsCacheableTest, SimpleRedirect) {
  // Single redirect.
  std::string url1 = "http://foo.com/";
  std::string url2 = "http://www.foo.com/";

  AddTemporaryRedirect(url1, url2);
  NewPrimaryResource(url2);
  Freeze();

  std::vector<std::string> urls;
  urls.push_back(url1);
  urls.push_back(url2);

  std::vector<Violation> violations;
  violations.push_back(Violation(1, urls));

  CheckViolations(violations);
}

TEST_F(MakeLandingPageRedirectsCacheableTest, EmptyLocation) {
  // Single redirect.
  std::string url1 = "http://foo.com/";
  std::string url2 = "http://www.foo.com/";
  std::string empty = "";

  AddTemporaryRedirect(url1, empty);
  NewPrimaryResource(url2);
  Freeze();

  // Though there is a 302, it does not redirect anywhere since it is
  // missing a location header. Thus, this should not be flagged as a
  // redirect. Perhaps it should be flagged in AvoidBadRequests.
  std::vector<Violation> violations;
  CheckViolations(violations);
}

TEST_F(MakeLandingPageRedirectsCacheableTest, NoRedirects) {
  // Single redirect.
  std::string url1 = "http://www.foo.com/";
  std::string url2 = "http://www.bar.com/";

  AddResourceUrl(url1, 200);
  NewPrimaryResource(url2);
  Freeze();

  std::vector<Violation> violations;
  CheckViolations(violations);
}

TEST_F(MakeLandingPageRedirectsCacheableTest, RedirectChain) {
  // Test longer chains.
  std::string url1 = "http://foo.com/";
  std::string url2 = "http://www.foo.com/";
  std::string url3 = "http://www.foo.com/index.html";

  AddTemporaryRedirect(url1, url2);
  AddTemporaryRedirect(url2, url3);
  NewPrimaryResource(url3);
  Freeze();

  std::vector<std::string> urls1;
  urls1.push_back(url1);
  urls1.push_back(url2);

  std::vector<std::string> urls2;
  urls2.push_back(url2);
  urls2.push_back(url3);

  std::vector<Violation> violations;
  violations.push_back(Violation(1, urls1));
  violations.push_back(Violation(1, urls2));

  CheckViolations(violations);
}

TEST_F(MakeLandingPageRedirectsCacheableTest, AbsoltutePath) {
  // Redirect given using an absolute path instead of fully qualified url.
  std::string url1 = "http://foo.com/";
  std::string url2 = "http://foo.com/a/b/pony.gif";
  std::string url3 = "http://foo.com/common/pony.gif";
  std::string url3_path = "/common/pony.gif";

  AddTemporaryRedirect(url1, url2);
  AddTemporaryRedirect(url2, url3_path);
  NewPrimaryResource(url3);
  Freeze();

  std::vector<std::string> urls1;
  urls1.push_back(url1);
  urls1.push_back(url2);

  std::vector<std::string> urls2;
  urls2.push_back(url2);
  urls2.push_back(url3);

  std::vector<Violation> violations;
  violations.push_back(Violation(1, urls1));
  violations.push_back(Violation(1, urls2));

  CheckViolations(violations);
}

TEST_F(MakeLandingPageRedirectsCacheableTest, RelativePath) {
  // Redirect given using a relative path instead of fully qualified url.
  std::string url1 = "http://foo.com/";
  std::string url2 = "http://foo.com/a/b/pony.gif";
  std::string url3 = "http://foo.com/a/b/common/pony.gif";
  std::string url3_relative = "common/pony.gif";

  AddTemporaryRedirect(url1, url2);
  AddTemporaryRedirect(url2, url3_relative);
  NewPrimaryResource(url3);
  Freeze();

  std::vector<std::string> urls1;
  urls1.push_back(url1);
  urls1.push_back(url2);

  std::vector<std::string> urls2;
  urls2.push_back(url2);
  urls2.push_back(url3);

  std::vector<Violation> violations;
  violations.push_back(Violation(1, urls1));
  violations.push_back(Violation(1, urls2));

  CheckViolations(violations);
}

TEST_F(MakeLandingPageRedirectsCacheableTest, Fragment) {
  // Redirect given using an absolute path instead of fully qualified url.
  std::string url1 = "http://foo.com/";
  std::string url2 = "http://foo.com/a/b/pony.gif";
  std::string url3 = "http://foo.com/common";
  std::string url3_with_fragment = "http://foo.com/common#frament";

  AddTemporaryRedirect(url1, url2);
  AddTemporaryRedirect(url2, url3_with_fragment);
  NewPrimaryResource(url3);
  Freeze();

  std::vector<std::string> urls1;
  urls1.push_back(url1);
  urls1.push_back(url2);

  std::vector<std::string> urls2;
  urls2.push_back(url2);
  urls2.push_back(url3);

  std::vector<Violation> violations;
  violations.push_back(Violation(1, urls1));
  violations.push_back(Violation(1, urls2));

  CheckViolations(violations);
}

TEST_F(MakeLandingPageRedirectsCacheableTest, SimpleRedirectPermanent) {
  std::string url1 = "http://foo.com/";
  std::string url2 = "http://www.foo.com/";

  AddPermanentRedirect(url1, url2);
  NewPrimaryResource(url2);
  Freeze();

  std::vector<std::string> urls;
  urls.push_back(url1);
  urls.push_back(url2);

  std::vector<Violation> violations;
  CheckViolations(violations);
}

TEST_F(MakeLandingPageRedirectsCacheableTest, PermanentAndTemp) {
  std::string url1 = "http://foo.com/";
  std::string url2 = "http://www.foo.com/";
  std::string url3 = "http://www.foo.com/common";

  AddPermanentRedirect(url1, url2);
  AddTemporaryRedirect(url2, url3);
  NewPrimaryResource(url3);
  Freeze();

  std::vector<std::string> urls;
  urls.push_back(url2);
  urls.push_back(url3);

  std::vector<Violation> violations;
  violations.push_back(Violation(1, urls));
  CheckViolations(violations);
}

TEST_F(MakeLandingPageRedirectsCacheableTest, TempAndPermanent) {
  std::string url1 = "http://foo.com/";
  std::string url2 = "http://www.foo.com/";
  std::string url3 = "http://www.foo.com/common";

  AddTemporaryRedirect(url1, url2);
  AddPermanentRedirect(url2, url3);
  NewPrimaryResource(url3);
  Freeze();

  std::vector<std::string> urls;
  urls.push_back(url1);
  urls.push_back(url2);

  std::vector<Violation> violations;
  violations.push_back(Violation(1, urls));
  CheckViolations(violations);
}

TEST_F(MakeLandingPageRedirectsCacheableTest, TwoNonCacheable) {
  std::string url1 = "http://foo.com/";
  std::string url2 = "http://www.foo.com/";
  std::string url3 = "http://www.foo.com/common";
  std::string url4 = "http://www.foo.com/common/";

  AddTemporaryRedirect(url1, url2);
  AddPermanentRedirect(url2, url3);
  AddTemporaryRedirect(url3, url4);
  NewPrimaryResource(url4);
  Freeze();

  std::vector<std::string> urls1;
  urls1.push_back(url1);
  urls1.push_back(url2);

  std::vector<std::string> urls2;
  urls2.push_back(url3);
  urls2.push_back(url4);

  std::vector<Violation> violations;
  violations.push_back(Violation(1, urls1));
  violations.push_back(Violation(1, urls2));
  CheckViolations(violations);
}

TEST_F(MakeLandingPageRedirectsCacheableTest, CacheableTempAndPermanent) {
  std::string url1 = "http://foo.com/";
  std::string url2 = "http://www.foo.com/";
  std::string url3 = "http://www.foo.com/common";

  AddCacheableTemporaryRedirect(url1, url2);
  AddPermanentRedirect(url2, url3);
  NewPrimaryResource(url3);
  Freeze();

  // No violation.
  std::vector<Violation> violations;
  CheckViolations(violations);
}

TEST_F(MakeLandingPageRedirectsCacheableTest, PrimaryResourceUrlHasFragment) {
  static const char *kUrlWithFragment = "http://www.example.com/foo#fragment";
  static const char *kUrlNoFragment = "http://www.example.com/foo";
  NewPrimaryResource(kUrlWithFragment);
  AddTemporaryRedirect(kUrl1, kUrlWithFragment);
  Freeze();

  // We expect that the resource's URL was converted to not have a
  // fragment.
  ASSERT_EQ(kUrlNoFragment, primary_resource()->GetRequestUrl());
  ASSERT_EQ(kUrlWithFragment, pagespeed_input()->primary_resource_url());
  ASSERT_EQ(
      pagespeed_input()->GetResourceWithUrlOrNull(
          kUrlWithFragment)->GetRequestUrl(),
      kUrlNoFragment);

  std::vector<std::string> urls;
  urls.push_back(kUrl1);
  urls.push_back(kUrlNoFragment);
  std::vector<Violation> violations;
  violations.push_back(Violation(1, urls));
  CheckViolations(violations);
}

TEST_F(MakeLandingPageRedirectsCacheableTest, IgnoreLoginPages) {
  static const char* kInitialUrl = "http://www.example.com/";
  static const char* kLoginUrl = "http://www.example.com/lOgIn?foo=bar";
  NewPrimaryResource(kLoginUrl);
  AddTemporaryRedirect(kInitialUrl, kLoginUrl);
  Freeze();

  // No violation.
  std::vector<Violation> violations;
  CheckViolations(violations);
}

TEST_F(MakeLandingPageRedirectsCacheableTest,
       IgnoreRedirectsWithPrevUrlInQueryString) {
  static const char* kInitialUrl = "http://www.example.com/";
  static const char* kOopsUrl =
      "http://www.example.com/oops?http://www.example.com/";
  NewPrimaryResource(kOopsUrl);
  AddTemporaryRedirect(kInitialUrl, kOopsUrl);
  Freeze();

  // No violation.
  std::vector<Violation> violations;
  CheckViolations(violations);
}

TEST_F(MakeLandingPageRedirectsCacheableTest, IgnoreRedirectsToErrorPages) {
  static const char* kInitialUrl = "http://www.example.com/";
  static const char* kErrorUrl = "http://www.example.com/foo";
  NewPrimaryResource(kErrorUrl)->SetResponseStatusCode(503);
  AddTemporaryRedirect(kInitialUrl, kErrorUrl);
  Freeze();

  // No violation.
  std::vector<Violation> violations;
  CheckViolations(violations);
}

}  // namespace
