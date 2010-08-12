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

#include "base/at_exit.h"
#include "base/scoped_ptr.h"
#include "pagespeed/core/pagespeed_input.h"
#include "pagespeed/core/resource.h"
#include "pagespeed/core/result_provider.h"
#include "pagespeed/proto/pagespeed_output.pb.h"
#include "pagespeed/rules/minimize_dns_lookups.h"
#include "pagespeed/testing/pagespeed_test.h"

using pagespeed::rules::MinimizeDnsLookups;
using pagespeed::PagespeedInput;
using pagespeed::Resource;
using pagespeed::Result;
using pagespeed::Results;
using pagespeed::ResultProvider;

namespace {

class MinimizeDnsTest : public ::pagespeed_testing::PagespeedTest {
 protected:
  Resource* AddTestResource(const std::string& url) {
    Resource* resource = new Resource;
    resource->SetRequestUrl(url);
    resource->SetRequestMethod("GET");
    resource->SetRequestProtocol("HTTP");
    resource->SetResponseStatusCode(200);
    resource->SetResponseProtocol("HTTP/1.1");
    AddResource(resource);
    return resource;
  }

  void CheckViolations(const std::vector<std::string>& expected_violations) {
    MinimizeDnsLookups dns_rule;

    Results results;
    ResultProvider provider(dns_rule, &results);
    dns_rule.AppendResults(*input(), &provider);
    ASSERT_EQ((expected_violations.size() >= 1) ? 1 : 0, results.results_size());

    std::vector<std::string> urls;
    for (int idx = 0; idx < results.results_size(); idx++) {
      const Result& result = results.results(idx);
      ASSERT_EQ("MinimizeDnsLookups", result.rule_name()) << idx;
      ASSERT_EQ(1, result.savings().dns_requests_saved());
      for (int i = 0; i < result.resource_urls_size(); i++) {
        urls.push_back(result.resource_urls(i));
      }
    }

    ASSERT_EQ(expected_violations.size(), urls.size());

    for (size_t idx = 0; idx < urls.size(); ++idx) {
      EXPECT_EQ(expected_violations[idx], urls[idx]);
    }
  }

 private:
  base::AtExitManager at_exit_manager_;
};

TEST_F(MinimizeDnsTest, OneUrlNoViolation) {
  const std::string url = "http://foo.com";

  AddTestResource(url);
  SetPrimaryResourceUrl(url);
  Freeze();

  std::vector<std::string> expected_violations;

  CheckViolations(expected_violations);
}

TEST_F(MinimizeDnsTest, OneLazyOneNotNoViolation) {
  const std::string url1 = "http://foo.com";
  const std::string url2 = "http://bar.com/baz.js";

  AddTestResource(url1);
  AddTestResource(url2)->SetLazyLoaded();
  SetPrimaryResourceUrl(url1);
  Freeze();

  std::vector<std::string> expected_violations;

  CheckViolations(expected_violations);
}

TEST_F(MinimizeDnsTest, OneLazyTwoNotTwoViolations) {
  const std::string url1 = "http://foo.com";
  const std::string url2 = "http://b.foo.com/baz.js";
  const std::string url3 = "http://c.foo.com/quux.js";

  AddTestResource(url1);
  AddTestResource(url2);
  AddTestResource(url3)->SetLazyLoaded();
  Freeze();

  std::vector<std::string> expected_violations;
  expected_violations.push_back(url2);
  expected_violations.push_back(url1);

  CheckViolations(expected_violations);
}

TEST_F(MinimizeDnsTest, TwoUrlsOneHostNoViolations) {
  const std::string url1 = "http://foo.com";
  const std::string url2 = "http://foo.com/favicon.ico";

  AddTestResource(url1);
  AddTestResource(url2);
  Freeze();

  std::vector<std::string> expected_violations;

  CheckViolations(expected_violations);
}

TEST_F(MinimizeDnsTest, TwoUrlsTwoViolations) {
  const std::string url1 = "http://foo.com";
  const std::string url2 = "http://a.foo.com/image.png";

  AddTestResource(url1);
  AddTestResource(url2);
  Freeze();

  std::vector<std::string> expected_violations;
  expected_violations.push_back(url2);
  expected_violations.push_back(url1);

  CheckViolations(expected_violations);
}

TEST_F(MinimizeDnsTest, ThreeUrlsOneViolation) {
  const std::string url1 = "http://foo.com";
  const std::string url2 = "http://foo.com/favicon.ico";
  const std::string url3 = "http://a.foo.com/image.png";

  AddTestResource(url1);
  AddTestResource(url2);
  AddTestResource(url3);
  Freeze();

  std::vector<std::string> expected_violations;
  expected_violations.push_back(url3);

  CheckViolations(expected_violations);
}

TEST_F(MinimizeDnsTest, MainResourceNoViolation) {
  const std::string url1 = "http://foo.com/";
  const std::string url2 = "http://a.foo.com/image.png";

  AddTestResource(url1);
  AddTestResource(url2);
  SetPrimaryResourceUrl(url1);
  Freeze();

  std::vector<std::string> expected_violations;
  expected_violations.push_back(url2);

  CheckViolations(expected_violations);
}

TEST_F(MinimizeDnsTest, ExcludeNumericIps) {
  const std::string url1 = "http://foo.com";
  const std::string url2 = "http://a.foo.com/image.png";
  const std::string url3 = "http://127.0.0.1/";

  AddTestResource(url1);
  AddTestResource(url2);
  AddTestResource(url3);
  Freeze();

  std::vector<std::string> expected_violations;
  expected_violations.push_back(url2);
  expected_violations.push_back(url1);

  CheckViolations(expected_violations);
}

}  // namespace
