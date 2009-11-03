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
#include "pagespeed/proto/pagespeed_output.pb.h"
#include "pagespeed/rules/minimize_dns_lookups.h"
#include "testing/gtest/include/gtest/gtest.h"

using pagespeed::rules::MinimizeDnsLookups;
using pagespeed::PagespeedInput;
using pagespeed::Resource;
using pagespeed::Result;
using pagespeed::Results;

namespace {

class MinimizeDnsTest : public ::testing::Test {
 protected:
  virtual void SetUp() {
    input_.reset(new PagespeedInput);
  }

  virtual void TearDown() {
    input_.reset();
  }

  void AddTestResource(const std::string& url) {
    Resource* resource = new Resource;
    resource->SetRequestUrl(url);
    resource->SetRequestMethod("GET");
    resource->SetRequestProtocol("HTTP");
    resource->SetResponseStatusCode(200);
    resource->SetResponseProtocol("HTTP/1.1");

    input_->AddResource(resource);
  }

  void CheckViolations(int expected_num_hosts,
                       int expected_dns_savings,
                       const std::vector<std::string>& expected_violations) {
    MinimizeDnsLookups dns_rule;

    Results results;
    dns_rule.AppendResults(*input_, &results);
    ASSERT_EQ(results.results_size(), expected_dns_savings);

    std::vector<std::string> urls;
    for (int idx = 0; idx < results.results_size(); idx++) {
      const Result& result = results.results(idx);
      ASSERT_EQ(result.rule_name(), "MinimizeDnsLookups") << idx;
      ASSERT_EQ(result.savings().dns_requests_saved(), 1);

      ASSERT_EQ(result.resource_urls_size(), 1);
      urls.push_back(result.resource_urls(0));
    }

    ASSERT_EQ(urls.size(), expected_violations.size());

    for (int idx = 0; idx < urls.size(); ++idx) {
      EXPECT_EQ(urls[idx], expected_violations[idx]);
    }
  }

 private:
  scoped_ptr<PagespeedInput> input_;
};

TEST_F(MinimizeDnsTest, OneUrlNoViolation) {
  std::string url = "http://foo.com";

  AddTestResource(url);

  std::vector<std::string> expected_violations;

  CheckViolations(1, 0, expected_violations);
}

TEST_F(MinimizeDnsTest, TwoUrlOneHostNoViolations) {
  std::string url1 = "http://foo.com";
  std::string url2 = url1 + "/favicon.ico";

  AddTestResource(url1);
  AddTestResource(url2);

  std::vector<std::string> expected_violations;

  CheckViolations(1, 0, expected_violations);
}

TEST_F(MinimizeDnsTest, TwoUrlTwoViolations) {
  std::string url1 = "http://bar.com/favicon.ico";
  std::string url2 = "http://foo.com";

  AddTestResource(url1);
  AddTestResource(url2);

  std::vector<std::string> expected_violations;
  expected_violations.push_back(url1);
  expected_violations.push_back(url2);

  CheckViolations(2, 2, expected_violations);
}

TEST_F(MinimizeDnsTest, TwoUrlOneViolation) {
  std::string url1 = "http://foo.com";
  std::string url2 = url1 + "/favicon.ico";
  std::string url3 = "http://bar.com/favicon.ico";

  AddTestResource(url1);
  AddTestResource(url2);
  AddTestResource(url3);

  std::vector<std::string> expected_violations;
  expected_violations.push_back(url3);

  CheckViolations(2, 1, expected_violations);
}

TEST_F(MinimizeDnsTest, ExcludeNumericIps) {
  std::string url1 = "http://bar.com/favicon.ico";
  std::string url2 = "http://127.0.0.1/";

  AddTestResource(url1);
  AddTestResource(url2);

  std::vector<std::string> expected_violations;
  expected_violations.push_back(url1);

  CheckViolations(2, 1, expected_violations);
}

}  // namespace
