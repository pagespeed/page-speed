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
using pagespeed::RuleResults;
using pagespeed::ResultProvider;

namespace {

class MinimizeDnsLookupsTest : public
    ::pagespeed_testing::PagespeedRuleTest<MinimizeDnsLookups> {
 protected:
  void CheckViolations(const std::vector<std::string>& expected_violations) {
    ASSERT_TRUE(AppendResults());
    ASSERT_EQ((expected_violations.size() >= 1) ? 1 : 0,
              rule_results().results_size());

    std::vector<std::string> urls;
    for (int idx = 0; idx < rule_results().results_size(); idx++) {
      const Result& result = rule_results().results(idx);
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
};

TEST_F(MinimizeDnsLookupsTest, OneUrlNoViolation) {
  NewPrimaryResource("http://foo.com/");
  Freeze();

  std::vector<std::string> expected_violations;

  CheckViolations(expected_violations);
}

TEST_F(MinimizeDnsLookupsTest, OnePostOnloadOneNotNoViolation) {
  const std::string url1 = "http://foo.com/";
  const std::string url2 = "http://bar.com/baz.js";

  SetOnloadTimeMillis(10);
  NewPrimaryResource(url1);
  New200Resource(url2)->SetRequestStartTimeMillis(11);
  Freeze();

  std::vector<std::string> expected_violations;

  CheckViolations(expected_violations);
}

TEST_F(MinimizeDnsLookupsTest, OnePostOnloadTwoNotTwoViolations) {
  const std::string url1 = "http://foo.com/";
  const std::string url2 = "http://b.foo.com/baz.js";
  const std::string url3 = "http://c.foo.com/quux.js";

  SetOnloadTimeMillis(10);
  New200Resource(url1);
  New200Resource(url2);
  New200Resource(url3)->SetRequestStartTimeMillis(11);
  Freeze();

  std::vector<std::string> expected_violations;
  expected_violations.push_back(url2);
  expected_violations.push_back(url1);

  CheckViolations(expected_violations);
}

TEST_F(MinimizeDnsLookupsTest, TwoUrlsOneHostNoViolations) {
  const std::string url1 = "http://foo.com/";
  const std::string url2 = "http://foo.com/favicon.ico";

  New200Resource(url1);
  New200Resource(url2);
  Freeze();

  std::vector<std::string> expected_violations;

  CheckViolations(expected_violations);
}

TEST_F(MinimizeDnsLookupsTest, TwoUrlsTwoViolations) {
  const std::string url1 = "http://foo.com/";
  const std::string url2 = "http://a.foo.com/image.png";

  New200Resource(url1);
  New200Resource(url2);
  Freeze();

  std::vector<std::string> expected_violations;
  expected_violations.push_back(url2);
  expected_violations.push_back(url1);

  CheckViolations(expected_violations);
}

TEST_F(MinimizeDnsLookupsTest, ThreeUrlsOneViolation) {
  const std::string url1 = "http://foo.com/";
  const std::string url2 = "http://foo.com/favicon.ico";
  const std::string url3 = "http://a.foo.com/image.png";

  New200Resource(url1);
  New200Resource(url2);
  New200Resource(url3);
  Freeze();

  std::vector<std::string> expected_violations;
  expected_violations.push_back(url3);

  CheckViolations(expected_violations);
}

TEST_F(MinimizeDnsLookupsTest, MainResourceNoViolation) {
  const std::string url1 = "http://foo.com/";
  const std::string url2 = "http://a.foo.com/image.png";

  NewPrimaryResource(url1);
  New200Resource(url2);
  Freeze();

  std::vector<std::string> expected_violations;
  expected_violations.push_back(url2);

  CheckViolations(expected_violations);
}

TEST_F(MinimizeDnsLookupsTest, ExcludeNumericIps) {
  const std::string url1 = "http://foo.com/";
  const std::string url2 = "http://a.foo.com/image.png";
  const std::string url3 = "http://127.0.0.1/";

  New200Resource(url1);
  New200Resource(url2);
  New200Resource(url3);
  Freeze();

  std::vector<std::string> expected_violations;
  expected_violations.push_back(url2);
  expected_violations.push_back(url1);

  CheckViolations(expected_violations);
}

TEST_F(MinimizeDnsLookupsTest, MainResourceRedirectChain) {
  const std::string url1 = "http://foo.com/";
  const std::string url2 = "http://a.foo.com/image.png";

  New302Resource(url1, url2);
  NewPrimaryResource(url2);
  Freeze();

  std::vector<std::string> expected_violations;
  CheckViolations(expected_violations);
}

}  // namespace
