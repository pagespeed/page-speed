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

#include <string>
#include <vector>

#include "base/logging.h"
#include "base/memory/scoped_ptr.h"
#include "pagespeed/core/pagespeed_input.h"
#include "pagespeed/core/resource.h"
#include "pagespeed/core/result_provider.h"
#include "pagespeed/core/string_util.h"
#include "pagespeed/proto/pagespeed_output.pb.h"
#include "pagespeed/rules/avoid_landing_page_redirects.h"
#include "pagespeed/testing/pagespeed_test.h"

using pagespeed::rules::AvoidLandingPageRedirects;
using pagespeed::PagespeedInput;
using pagespeed::RedirectionDetails;
using pagespeed::Resource;
using pagespeed::ResourceType;
using pagespeed::Result;
using pagespeed::ResultDetails;
using pagespeed::Results;
using pagespeed::ResultProvider;
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

class AvoidLandingPageRedirectsTest : public
    ::pagespeed_testing::PagespeedRuleTest<AvoidLandingPageRedirects>  {
 public:
  AvoidLandingPageRedirectsTest()
      : request_start_time_millis_(0) {}

 private:
  // To enforce resources added in request order.
  int request_start_time_millis_;
 protected:
  void AddResourceUrl(const std::string& url, int status_code) {
    Resource* resource = new Resource;
    resource->SetRequestUrl(url);
    resource->SetRequestMethod("GET");
    resource->SetResponseStatusCode(status_code);
    resource->SetRequestStartTimeMillis(request_start_time_millis_++);
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
    resource->SetRequestStartTimeMillis(request_start_time_millis_++);
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

  Resource* SetPrimaryResource(const std::string& url) {
    Resource* resource = NewPrimaryResource(url);
    resource->SetRequestStartTimeMillis(request_start_time_millis_++);
    return resource;
  }

  void CheckViolations(const std::vector<Violation>& expected_violations) {
    ASSERT_TRUE(AppendResults());
    ASSERT_EQ(static_cast<size_t>(num_results()), expected_violations.size());
    for (int idx = 0; idx < num_results(); idx++) {
      const Result* r = &result(idx);
      const Violation& violation = expected_violations[idx];

      ASSERT_EQ(violation.expected_request_savings,
                r->savings().requests_saved());

      ASSERT_EQ(violation.urls.size(),
                static_cast<size_t>(r->resource_urls_size()));

      for (int url_idx = 0;
           url_idx < r->resource_urls_size();
           ++url_idx) {
        EXPECT_EQ(violation.urls[url_idx],
                  r->resource_urls(url_idx))
            << "At index: " << url_idx;
      }
    }
  }

  const RedirectionDetails& details(int result_idx) {
    pagespeed_testing::AssertTrue(result(result_idx).has_details());
    const ResultDetails& detail = result(result_idx).details();
    pagespeed_testing::AssertTrue(detail.HasExtension(
        RedirectionDetails::message_set_extension));
    return detail.GetExtension(RedirectionDetails::message_set_extension);
  }
};

TEST_F(AvoidLandingPageRedirectsTest, SimpleRedirect) {
  // Single redirect.
  std::string url1 = "http://foo.com/";
  std::string url2 = "http://www.foo.com/";

  AddTemporaryRedirect(url1, url2);
  SetPrimaryResource(url2);
  Freeze();

  std::vector<std::string> urls;
  urls.push_back(url1);
  urls.push_back(url2);

  std::vector<Violation> violations;
  violations.push_back(Violation(1, urls));

  CheckViolations(violations);
}

TEST_F(AvoidLandingPageRedirectsTest, EmptyLocation) {
  // Single redirect.
  std::string url1 = "http://foo.com/";
  std::string url2 = "http://www.foo.com/";
  std::string empty = "";

  AddTemporaryRedirect(url1, empty);
  SetPrimaryResource(url2);
  Freeze();

  // Although there is an empty redirection, we treat is as missing Location
  // headers. If the resource is added before the primary resource, we flag it
  // on the redirect chain.
  std::vector<std::string> urls1;
  urls1.push_back(url1);
  urls1.push_back(url2);

  std::vector<Violation> violations;
  violations.push_back(Violation(1, urls1));

  CheckViolations(violations);
}

TEST_F(AvoidLandingPageRedirectsTest, PermanentEmptyLocation) {
  // Single redirect.
  std::string url1 = "http://foo.com/";
  std::string url2 = "http://www.foo.com/";
  std::string url3 = "http://www.bar.com/";
  std::string empty = "";

  AddPermanentRedirect(url1, empty);
  AddTemporaryRedirect(url2, url3);
  SetPrimaryResource(url3);
  Freeze();

  // Although there is an empty redirection, we treat is as missing Location
  // headers. If the resource is added before the primary resource, we flag it
  // on the redirect chain.
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

TEST_F(AvoidLandingPageRedirectsTest, NoRedirects) {
  // Single redirect.
  std::string url1 = "http://www.foo.com/";
  std::string url2 = "http://www.bar.com/";

  AddResourceUrl(url1, 200);
  SetPrimaryResource(url2);
  Freeze();

  std::vector<Violation> violations;
  CheckViolations(violations);
}

TEST_F(AvoidLandingPageRedirectsTest, RedirectChain) {
  // Test longer chains.
  std::string url1 = "http://foo.com/";
  std::string url2 = "http://www.foo.com/";
  std::string url3 = "http://www.foo.com/index.html";

  AddTemporaryRedirect(url1, url2);
  AddTemporaryRedirect(url2, url3);
  SetPrimaryResource(url3);
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

TEST_F(AvoidLandingPageRedirectsTest, AbsoltutePath) {
  // Redirect given using an absolute path instead of fully qualified url.
  std::string url1 = "http://foo.com/";
  std::string url2 = "http://foo.com/a/b/pony.gif";
  std::string url3 = "http://foo.com/common/pony.gif";
  std::string url3_path = "/common/pony.gif";

  AddTemporaryRedirect(url1, url2);
  AddTemporaryRedirect(url2, url3_path);
  SetPrimaryResource(url3);
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

TEST_F(AvoidLandingPageRedirectsTest, RelativePath) {
  // Redirect given using a relative path instead of fully qualified url.
  std::string url1 = "http://foo.com/";
  std::string url2 = "http://foo.com/a/b/pony.gif";
  std::string url3 = "http://foo.com/a/b/common/pony.gif";
  std::string url3_relative = "common/pony.gif";

  AddTemporaryRedirect(url1, url2);
  AddTemporaryRedirect(url2, url3_relative);
  SetPrimaryResource(url3);
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

TEST_F(AvoidLandingPageRedirectsTest, Fragment) {
  // Redirect given using an absolute path instead of fully qualified url.
  std::string url1 = "http://foo.com/";
  std::string url2 = "http://foo.com/a/b/pony.gif";
  std::string url3 = "http://foo.com/common";
  std::string url3_with_fragment = "http://foo.com/common#frament";

  AddTemporaryRedirect(url1, url2);
  AddTemporaryRedirect(url2, url3_with_fragment);
  SetPrimaryResource(url3);
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

TEST_F(AvoidLandingPageRedirectsTest, SimpleRedirectPermanent) {
  std::string url1 = "http://foo.com/";
  std::string url2 = "http://www.foo.com/";

  AddPermanentRedirect(url1, url2);
  SetPrimaryResource(url2);
  Freeze();

  std::vector<std::string> urls;
  urls.push_back(url1);
  urls.push_back(url2);

  std::vector<Violation> violations;
  violations.push_back(Violation(1, urls));
  CheckViolations(violations);
}

TEST_F(AvoidLandingPageRedirectsTest, PermanentAndTemp) {
  std::string url1 = "http://foo.com/";
  std::string url2 = "http://www.foo.com/";
  std::string url3 = "http://www.foo.com/common";

  AddPermanentRedirect(url1, url2);
  AddTemporaryRedirect(url2, url3);
  SetPrimaryResource(url3);
  Freeze();

  std::vector<std::string> urls;
  urls.push_back(url1);
  urls.push_back(url2);

  std::vector<std::string> urls2;
  urls2.push_back(url2);
  urls2.push_back(url3);


  std::vector<Violation> violations;
  violations.push_back(Violation(1, urls));
  violations.push_back(Violation(1, urls2));
  CheckViolations(violations);
}

TEST_F(AvoidLandingPageRedirectsTest, TempAndPermanent) {
  std::string url1 = "http://foo.com/";
  std::string url2 = "http://www.foo.com/";
  std::string url3 = "http://www.foo.com/common";

  AddTemporaryRedirect(url1, url2);
  AddPermanentRedirect(url2, url3);
  SetPrimaryResource(url3);
  Freeze();

  std::vector<std::string> urls;
  urls.push_back(url1);
  urls.push_back(url2);

  std::vector<std::string> urls2;
  urls2.push_back(url2);
  urls2.push_back(url3);

  std::vector<Violation> violations;
  violations.push_back(Violation(1, urls));
  violations.push_back(Violation(1, urls2));
  CheckViolations(violations);
}

TEST_F(AvoidLandingPageRedirectsTest, TwoNonCacheable) {
  std::string url1 = "http://foo.com/";
  std::string url2 = "http://www.foo.com/";
  std::string url3 = "http://www.foo.com/common";
  std::string url4 = "http://www.foo.com/common/";

  AddTemporaryRedirect(url1, url2);
  AddPermanentRedirect(url2, url3);
  AddTemporaryRedirect(url3, url4);
  SetPrimaryResource(url4);
  Freeze();

  std::vector<std::string> urls1;
  urls1.push_back(url1);
  urls1.push_back(url2);

  std::vector<std::string> urls2;
  urls2.push_back(url2);
  urls2.push_back(url3);

  std::vector<std::string> urls3;
  urls3.push_back(url3);
  urls3.push_back(url4);

  std::vector<Violation> violations;
  violations.push_back(Violation(1, urls1));
  violations.push_back(Violation(1, urls2));
  violations.push_back(Violation(1, urls3));
  CheckViolations(violations);
}

TEST_F(AvoidLandingPageRedirectsTest, CacheableTempAndPermanent) {
  std::string url1 = "http://foo.com/";
  std::string url2 = "http://www.foo.com/";
  std::string url3 = "http://www.foo.com/common";

  AddCacheableTemporaryRedirect(url1, url2);
  AddPermanentRedirect(url2, url3);
  SetPrimaryResource(url3);
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

TEST_F(AvoidLandingPageRedirectsTest, PrimaryResourceUrlHasFragment) {
  static const char *kUrlWithFragment = "http://www.example.com/foo#fragment";
  static const char *kUrlNoFragment = "http://www.example.com/foo";
  AddTemporaryRedirect(kUrl1, kUrlWithFragment);
  SetPrimaryResource(kUrlWithFragment);
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

TEST_F(AvoidLandingPageRedirectsTest, LoginPages) {
  static const char* kInitialUrl = "http://www.example.com/";
  static const char* kLoginUrl = "http://www.example.com/lOgIn?foo=bar";
  AddTemporaryRedirect(kInitialUrl, kLoginUrl);
  SetPrimaryResource(kLoginUrl);
  Freeze();

  std::vector<std::string> urls;
  urls.push_back(kInitialUrl);
  urls.push_back(kLoginUrl);

  // One violation.
  std::vector<Violation> violations;
  violations.push_back(Violation(1, urls));
  CheckViolations(violations);

  RedirectionDetails detail = details(0);
  ASSERT_TRUE(detail.has_is_likely_login());
  ASSERT_TRUE(detail.is_likely_login());
}

TEST_F(AvoidLandingPageRedirectsTest,
       RedirectsWithPrevUrlInQueryString) {
  static const char* kInitialUrl = "http://www.example.com/";
  static const char* kOopsUrl =
      "http://www.example.com/oops?http://www.example.com/";
  AddTemporaryRedirect(kInitialUrl, kOopsUrl);
  SetPrimaryResource(kOopsUrl);
  Freeze();

  std::vector<std::string> urls;
  urls.push_back(kInitialUrl);
  urls.push_back(kOopsUrl);

  // One violation.
  std::vector<Violation> violations;
  violations.push_back(Violation(1, urls));
  CheckViolations(violations);

  RedirectionDetails detail = details(0);
  ASSERT_FALSE(detail.is_likely_login());
  ASSERT_TRUE(detail.is_likely_callback());
}

TEST_F(AvoidLandingPageRedirectsTest, IgnoreRedirectsToErrorPages) {
  static const char* kInitialUrl = "http://www.example.com/";
  static const char* kErrorUrl = "http://www.example.com/foo";
  SetPrimaryResource(kErrorUrl)->SetResponseStatusCode(503);
  AddTemporaryRedirect(kInitialUrl, kErrorUrl);
  Freeze();

  // No violation.
  std::vector<Violation> violations;
  CheckViolations(violations);
}

TEST_F(AvoidLandingPageRedirectsTest, FormatWithOrder) {
  std::string url1 = "http://foo.com/";
  std::string url2 = "http://www.foo.com/";
  std::string url3 = "http://www.foo.com/bar";
  std::string url4 = "http://www.bar.com/";
  std::string url5 = "http://www.bar.com/mobile";
  std::string url6 = "http://m.www.bar.com/";

  AddTemporaryRedirect(url1, url2);
  AddCacheableTemporaryRedirect(url2, url3);
  AddPermanentRedirect(url3, url4);
  AddRedirect(url4, 302, url5, "max-age=600");
  // Add a 301 redirect, but with an explicit cache control, it will be short
  // cacheable instead of permanent.
  AddRedirect(url5, 301, url6, "max-age=600");
  SetPrimaryResource(url6);
  Freeze();

  std::vector<std::string> redirection1;
  redirection1.push_back(url1);
  redirection1.push_back(url2);

  std::vector<std::string> redirection2;
  redirection2.push_back(url2);
  redirection2.push_back(url3);

  std::vector<std::string> redirection3;
  redirection3.push_back(url3);
  redirection3.push_back(url4);

  std::vector<std::string> redirection4;
  redirection4.push_back(url4);
  redirection4.push_back(url5);

  std::vector<std::string> redirection5;
  redirection5.push_back(url5);
  redirection5.push_back(url6);

  std::vector<Violation> violations;
  violations.push_back(Violation(1, redirection1));
  violations.push_back(Violation(1, redirection2));
  violations.push_back(Violation(1, redirection3));
  violations.push_back(Violation(1, redirection4));
  violations.push_back(Violation(1, redirection5));
  CheckViolations(violations);

  const char* expected_results =
      "To speed up page load times for visitors of your site, remove as many "
      "landing page redirections as possible, and make any required "
      "redirections cacheable if possible.\n"
      "  http://foo.com/ is a non-cacheable redirect to http://www.foo.com/\n"
      "  http://www.foo.com/ is a cacheable (1 year) redirect to "
      "http://www.foo.com/bar\n"
      "  http://www.foo.com/bar is a permanent redirect to "
      "http://www.bar.com/\n"
      "  http://www.bar.com/ is a short-cacheable (10 minutes) redirect to "
      "http://www.bar.com/mobile\n"
      "  http://www.bar.com/mobile is a short-cacheable (10 minutes) redirect "
      "to http://m.www.bar.com/\n";
  ASSERT_EQ(expected_results, FormatResults());
}

}  // namespace
