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
#include "pagespeed/core/pagespeed_input.pb.h"
#include "pagespeed/core/pagespeed_output.pb.h"
#include "pagespeed/rules/minimize_dns_details.pb.h"
#include "pagespeed/rules/minimize_dns_rule.h"
#include "testing/gtest/include/gtest/gtest.h"

using pagespeed::MinimizeDnsDetails;
using pagespeed::MinimizeDnsRule;
using pagespeed::PagespeedInput;
using pagespeed::ProtoInput;
using pagespeed::ProtoResource;
using pagespeed::Result;
using pagespeed::ResultDetails;
using pagespeed::Results;

namespace {

class MinimizeDnsTest : public ::testing::Test {
 protected:
  virtual void SetUp() {
    proto_input_.reset(new ProtoInput);
  }

  virtual void TearDown() {
    proto_input_.reset();
  }

  void AddTestResource(const std::string& url) {
    ProtoResource* proto_resource = proto_input_->add_resources();
    proto_resource->set_request_url(url);
    proto_resource->set_request_method("GET");
    proto_resource->set_request_protocol("HTTP");
    proto_resource->set_response_status_code(200);
    proto_resource->set_response_protocol("HTTP/1.1");
  }

  void CheckViolations(int expected_num_hosts,
                       int expected_dns_savings,
                       const std::vector<std::string>& expected_violations) {
    PagespeedInput input(proto_input_.get());
    MinimizeDnsRule dns_rule;

    Results results;
    dns_rule.AppendResults(input, &results);
    ASSERT_EQ(results.results_size(), 1);

    const Result& result = results.results(0);
    ASSERT_EQ(result.rule_name(), "MinimizeDnsRule");
    ASSERT_EQ(result.savings().dns_requests_saved(), expected_dns_savings);

    const ResultDetails& details = result.details();
    const MinimizeDnsDetails& dns_details = details.GetExtension(
        MinimizeDnsDetails::message_set_extension);

    EXPECT_EQ(dns_details.num_hosts(), expected_num_hosts);
    ASSERT_EQ(dns_details.violation_urls_size(), expected_violations.size());

    for (int idx = 0; idx < dns_details.violation_urls_size(); ++idx) {
      EXPECT_EQ(dns_details.violation_urls(idx), expected_violations[idx]);
      }
  }

 private:
  scoped_ptr<ProtoInput> proto_input_;
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
