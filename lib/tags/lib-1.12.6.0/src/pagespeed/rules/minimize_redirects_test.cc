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
#include "base/scoped_ptr.h"
#include "pagespeed/core/pagespeed_input.h"
#include "pagespeed/core/resource.h"
#include "pagespeed/core/result_provider.h"
#include "pagespeed/proto/pagespeed_output.pb.h"
#include "pagespeed/rules/minimize_redirects.h"
#include "pagespeed/testing/pagespeed_test.h"

using pagespeed::rules::MinimizeRedirects;
using pagespeed::PagespeedInput;
using pagespeed::Resource;
using pagespeed::ResourceType;
using pagespeed::Result;
using pagespeed::Results;
using pagespeed::ResultProvider;
using pagespeed::Rule;
using pagespeed::RuleResults;

namespace {

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

class MinimizeRedirectsTest : public ::pagespeed_testing::PagespeedTest {
 protected:
  void AddResourceUrl(const std::string& url, int status_code) {
    Resource* resource = new Resource;
    resource->SetRequestUrl(url);
    resource->SetRequestMethod("GET");
    resource->SetResponseStatusCode(status_code);
    AddResource(resource);
  }

  void AddRedirect(const std::string& url, const std::string& location) {
    Resource* resource = new Resource;
    resource->SetRequestUrl(url);
    resource->SetRequestMethod("GET");
    resource->SetResponseStatusCode(302);
    if (!location.empty()) {
      resource->AddResponseHeader("Location", location);
    }
    AddResource(resource);
  }

  void CheckViolations(const std::vector<Violation>& expected_violations) {
    MinimizeRedirects rule;

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

TEST_F(MinimizeRedirectsTest, SimpleRedirect) {
  // Single redirect.
  std::string url1 = "http://foo.com/";
  std::string url2 = "http://www.foo.com/";

  AddRedirect(url1, url2);
  AddResourceUrl(url2, 200);
  Freeze();

  std::vector<std::string> urls;
  urls.push_back(url1);
  urls.push_back(url2);

  std::vector<Violation> violations;
  violations.push_back(Violation(1, urls));

  CheckViolations(violations);
}

TEST_F(MinimizeRedirectsTest, EmptyLocation) {
  // Single redirect.
  std::string url1 = "http://foo.com/";
  std::string url2 = "http://www.foo.com/";
  std::string empty = "";

  AddRedirect(url1, empty);
  AddResourceUrl(url2, 200);
  Freeze();

  // Though there is a 302, it does not redirect anywhere since it is
  // missing a location header. Thus, this should not be flagged as a
  // redirect. Perhaps it should be flagged in AvoidBadRequests.
  std::vector<Violation> violations;
  CheckViolations(violations);
}

TEST_F(MinimizeRedirectsTest, NoRedirects) {
  // Single redirect.
  std::string url1 = "http://www.foo.com/";
  std::string url2 = "http://www.bar.com/";

  AddResourceUrl(url1, 200);
  AddResourceUrl(url2, 304);
  Freeze();

  std::vector<Violation> violations;

  CheckViolations(violations);
}

TEST_F(MinimizeRedirectsTest, RedirectChain) {
  // Test longer chains.
  std::string url1 = "http://foo.com/";
  std::string url2 = "http://www.foo.com/";
  std::string url3 = "http://www.foo.com/index.html";

  AddRedirect(url1, url2);
  AddRedirect(url2, url3);
  AddResourceUrl(url3, 200);
  Freeze();

  std::vector<std::string> urls;
  urls.push_back(url1);
  urls.push_back(url2);
  urls.push_back(url3);

  std::vector<Violation> violations;
  violations.push_back(Violation(2, urls));

  CheckViolations(violations);
}

TEST_F(MinimizeRedirectsTest, AbsoltutePath) {
  // Redirect given using an absolute path instead of fully qualified url.
  std::string url1 = "http://foo.com/";
  std::string url2 = "http://foo.com/a/b/pony.gif";
  std::string url3 = "http://foo.com/common/pony.gif";
  std::string url3_path = "/common/pony.gif";

  AddRedirect(url1, url2);
  AddRedirect(url2, url3_path);
  AddResourceUrl(url3, 200);
  Freeze();

  std::vector<std::string> urls;
  urls.push_back(url1);
  urls.push_back(url2);
  urls.push_back(url3);

  std::vector<Violation> violations;
  violations.push_back(Violation(2, urls));

  CheckViolations(violations);
}

TEST_F(MinimizeRedirectsTest, RelativePath) {
  // Redirect given using a relative path instead of fully qualified url.
  std::string url1 = "http://foo.com/";
  std::string url2 = "http://foo.com/a/b/pony.gif";
  std::string url3 = "http://foo.com/a/b/common/pony.gif";
  std::string url3_relative = "common/pony.gif";

  AddRedirect(url1, url2);
  AddRedirect(url2, url3_relative);
  AddResourceUrl(url3, 200);
  Freeze();

  std::vector<std::string> urls;
  urls.push_back(url1);
  urls.push_back(url2);
  urls.push_back(url3);

  std::vector<Violation> violations;
  violations.push_back(Violation(2, urls));

  CheckViolations(violations);
}

TEST_F(MinimizeRedirectsTest, Fragment) {
  // Redirect given using an absolute path instead of fully qualified url.
  std::string url1 = "http://foo.com/";
  std::string url2 = "http://foo.com/a/b/pony.gif";
  std::string url3 = "http://foo.com/common";
  std::string url3_with_fragment = "http://foo.com/common#frament";

  AddRedirect(url1, url2);
  AddRedirect(url2, url3_with_fragment);
  AddResourceUrl(url3, 200);
  Freeze();

  std::vector<std::string> urls;
  urls.push_back(url1);
  urls.push_back(url2);
  urls.push_back(url3);

  std::vector<Violation> violations;
  violations.push_back(Violation(2, urls));

  CheckViolations(violations);
}

TEST_F(MinimizeRedirectsTest, RedirectGraphNoCycles1) {
  // Graph with multiple roots.
  std::string url1 = "http://foo.com/";
  std::string url2 = "http://www.foo.com/";
  std::string url3 = "http://www.foo.com/index.html";
  std::string url4 = "http://bar.com/";

  AddRedirect(url1, url2);
  AddRedirect(url2, url3);
  AddResourceUrl(url3, 200);
  AddRedirect(url4, url3);
  Freeze();

  std::vector<std::string> urls1;
  urls1.push_back(url1);
  urls1.push_back(url2);
  urls1.push_back(url3);

  std::vector<std::string> urls2;
  urls2.push_back(url4);
  urls2.push_back(url3);

  std::vector<Violation> violations;
  violations.push_back(Violation(1, urls2));
  violations.push_back(Violation(2, urls1));

  CheckViolations(violations);
}

TEST_F(MinimizeRedirectsTest, RedirectGraphCycles) {
  // break cycles that are part of rooted graphs arbitrarily.
  std::string url1 = "http://foo.com/";
  std::string url2 = "http://www.foo.com/";
  std::string url3 = "http://www.bar.com/";
  std::string url4 = "http://www.baz.com/";

  AddRedirect(url1, url2);
  AddRedirect(url2, url3);
  AddRedirect(url3, url4);
  AddRedirect(url4, url2);
  Freeze();

  std::vector<std::string> urls;
  urls.push_back(url1);
  urls.push_back(url2);
  urls.push_back(url3);
  urls.push_back(url4);
  urls.push_back(url2);

  std::vector<Violation> violations;
  violations.push_back(Violation(4, urls));

  CheckViolations(violations);
}

TEST_F(MinimizeRedirectsTest, RedirectGraphCycles2) {
  // break cycles that are part of rooted graphs arbitrarily.
  std::string url1 = "http://foo.com/";
  std::string url2 = "http://www.foo.com/";
  std::string url3 = "http://www.bar.com/";
  std::string url4 = "http://www.baz.com/";
  std::string url5 = "http://a.com/";

  AddRedirect(url1, url2);
  AddRedirect(url2, url3);
  AddRedirect(url3, url4);
  AddRedirect(url4, url2);
  AddRedirect(url5, url3);
  Freeze();

  std::vector<std::string> urls1;
  urls1.push_back(url5);
  urls1.push_back(url3);
  urls1.push_back(url4);
  urls1.push_back(url2);
  urls1.push_back(url3);

  std::vector<std::string> urls2;
  urls2.push_back(url1);
  urls2.push_back(url2);

  std::vector<Violation> violations;
  violations.push_back(Violation(4, urls1));
  violations.push_back(Violation(1, urls2));

  CheckViolations(violations);
}

TEST_F(MinimizeRedirectsTest, RedirectCycles) {
  // pure redirect cycle; currently not checked.
  std::string url1 = "http://www.a.com/";
  std::string url2 = "http://www.b.com/";
  std::string url3 = "http://www.c.com/";

  AddRedirect(url1, url2);
  AddRedirect(url2, url3);
  AddRedirect(url3, url1);
  Freeze();

  std::vector<std::string> urls;
  urls.push_back(url1);
  urls.push_back(url2);
  urls.push_back(url3);
  urls.push_back(url1);

  std::vector<Violation> violations;
  violations.push_back(Violation(3, urls));

  CheckViolations(violations);
}

TEST_F(MinimizeRedirectsTest, MissingDestination) {
  // pure redirect cycle; currently not checked.
  std::string url1 = "http://www.a.com/";
  std::string url2 = "http://www.b.com/";

  AddRedirect(url1, url2);
  Freeze();

  std::vector<Violation> violations;
  CheckViolations(violations);
}

TEST_F(MinimizeRedirectsTest, LandingPageRedirect) {
  // Single redirect.
  std::string url1 = "http://foo.com/";
  std::string url2 = "http://www.foo.com/";

  AddRedirect(url1, url2);
  NewPrimaryResource(url2);
  Freeze();

  std::vector<std::string> urls;
  urls.push_back(url1);
  urls.push_back(url2);

  // No violations.
  std::vector<Violation> violations;
  CheckViolations(violations);
}

}  // namespace
