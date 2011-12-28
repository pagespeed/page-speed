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
#include "pagespeed/core/rule_input.h"
#include "pagespeed/proto/pagespeed_output.pb.h"
#include "pagespeed/testing/pagespeed_test.h"

using pagespeed::PagespeedInput;
using pagespeed::Resource;
using pagespeed::ResourceType;
using pagespeed::Result;
using pagespeed::Results;
using pagespeed::ResultProvider;
using pagespeed::Rule;
using pagespeed::RuleInput;
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

class RuleInputTest : public ::pagespeed_testing::PagespeedTest {
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
    pagespeed::RuleInput rule_input(*pagespeed_input());
    rule_input.Init();
    const RuleInput::RedirectChainVector& redirect_chains =
      rule_input.GetRedirectChains();

    ASSERT_EQ(redirect_chains.size(), expected_violations.size());
    for (size_t idx = 0; idx < redirect_chains.size(); idx++) {
      const RuleInput::RedirectChain& chain = redirect_chains[idx];
      const Violation& violation = expected_violations[idx];

      ASSERT_EQ(violation.urls.size(), chain.size());

      for (size_t url_idx = 0;
           url_idx < chain.size();
           ++url_idx) {
        EXPECT_EQ(violation.urls[url_idx], chain[url_idx]->GetRequestUrl())
            << "At index: " << url_idx;
      }
    }
  }
};

TEST_F(RuleInputTest, SimpleRedirect) {
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

TEST_F(RuleInputTest, RedirectChain) {
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
  violations.push_back(Violation(1, urls));

  CheckViolations(violations);
}

TEST_F(RuleInputTest, NoRedirect) {
  std::string url1 = "http://foo.com/";
  std::string url2 = "http://www.foo.com/";

  AddResourceUrl(url1, 200);
  AddResourceUrl(url2, 200);
  Freeze();

  std::vector<Violation> violations;
  CheckViolations(violations);
}

TEST_F(RuleInputTest, MissingDestination) {
  std::string url1 = "http://foo.com/";
  std::string url2 = "http://www.foo.com/";
  AddRedirect(url1, url2);
  Freeze();

  std::vector<std::string> urls;
  urls.push_back(url1);

  std::vector<Violation> violations;
  violations.push_back(Violation(1, urls));

  CheckViolations(violations);
}

TEST_F(RuleInputTest, FinalRedirectTarget) {
  std::string url1 = "http://foo.com/";
  std::string url2 = "http://www.foo.com/";
  std::string url3 = "http://www.foo.com/index.html";

  AddRedirect(url1, url2);
  AddRedirect(url2, url3);
  AddResourceUrl(url3, 200);
  Freeze();

  const PagespeedInput* input = pagespeed_input();
  RuleInput rule_input(*input);
  rule_input.Init();

  const Resource* resource1 = input->GetResourceWithUrlOrNull(url1);
  ASSERT_TRUE(NULL != resource1);
  const Resource* resource2 = input->GetResourceWithUrlOrNull(url2);
  ASSERT_TRUE(NULL != resource2);
  const Resource* resource3 = input->GetResourceWithUrlOrNull(url3);
  ASSERT_TRUE(NULL != resource3);
  EXPECT_EQ(resource3, rule_input.GetFinalRedirectTarget(resource1));
  EXPECT_EQ(resource3, rule_input.GetFinalRedirectTarget(resource2));
  EXPECT_EQ(resource3, rule_input.GetFinalRedirectTarget(resource3));
  EXPECT_EQ(NULL, rule_input.GetFinalRedirectTarget(NULL));
}

}  // namespace
