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
#include "pagespeed/core/pagespeed_input.pb.h"
#include "pagespeed/core/pagespeed_output.pb.h"
#include "pagespeed/core/proto_resource_utils.h"
#include "pagespeed/core/resource.h"
#include "pagespeed/rules/minimize_resources_details.pb.h"
#include "pagespeed/rules/minimize_resources_rule.h"
#include "testing/gtest/include/gtest/gtest.h"

using pagespeed::MinimizeCssResourcesRule;
using pagespeed::MinimizeJsResourcesRule;
using pagespeed::MinimizeResourcesDetails;
using pagespeed::PagespeedInput;
using pagespeed::ProtoInput;
using pagespeed::ProtoResource;
using pagespeed::ProtoResourceUtils;
using pagespeed::ResourceType;
using pagespeed::Result;
using pagespeed::ResultDetails;
using pagespeed::Results;
using pagespeed::Rule;

namespace {

class MinimizeResourcesTest : public ::testing::Test {
 protected:
  virtual void SetUp() {
    proto_input_.reset(new ProtoInput);
  }

  virtual void TearDown() {
    proto_input_.reset();
  }

  void AddTestResource(const std::string& url,
                       const std::string& content_type) {
    ProtoResource* proto_resource = proto_input_->add_resources();
    proto_resource->set_request_url(url);
    ProtoResourceUtils::AddResponseHeader(
        proto_resource, "Content-Type", content_type);

    proto_resource->set_request_method("GET");
    proto_resource->set_request_protocol("HTTP");
    proto_resource->set_response_status_code(200);
    proto_resource->set_response_protocol("HTTP/1.1");
  }

  void CheckViolations(ResourceType type,
                       int expected_rt_savings,
                       int expected_violation_hosts,
                       const std::vector<std::string>& expected_violations) {
    PagespeedInput input(proto_input_.get());

    scoped_ptr<Rule> resource_rule;
    const char* rule_name = NULL;
    if (type == pagespeed::CSS) {
      resource_rule.reset(new MinimizeCssResourcesRule());
      rule_name = "MinimizeCssResourcesRule";
    } else if (type == pagespeed::JS) {
      resource_rule.reset(new MinimizeJsResourcesRule());
      rule_name = "MinimizeJsResourcesRule";
    } else {
      CHECK(false);
    }

    Results results;
    resource_rule->AppendResults(input, &results);
    ASSERT_EQ(results.results_size(), 1);
    const Result* result = &results.results(0);

    ASSERT_EQ(result->rule_name(), rule_name);
    ASSERT_EQ(result->savings().requests_saved(), expected_rt_savings);

    const ResultDetails& details = result->details();
    const pagespeed::MinimizeResourcesDetails& resource_details =
        details.GetExtension(
            pagespeed::MinimizeResourcesDetails::message_set_extension);

    EXPECT_EQ(resource_details.num_violation_hosts(), expected_violation_hosts);
    ASSERT_EQ(resource_details.violation_urls_size(),
              expected_violations.size());

    for (int idx = 0; idx < resource_details.violation_urls_size(); ++idx) {
      EXPECT_EQ(resource_details.violation_urls(idx),
                expected_violations[idx]);
                }
  }

 private:
  scoped_ptr<ProtoInput> proto_input_;
};

TEST_F(MinimizeResourcesTest, OneUrlNoViolation) {
  std::string url = "http://foo.com";

  AddTestResource(url, "text/css");

  std::vector<std::string> no_violations;

  CheckViolations(pagespeed::JS, 0, 0, no_violations);
  CheckViolations(pagespeed::CSS, 0, 0, no_violations);
}

TEST_F(MinimizeResourcesTest, TwoCssResourcesFromOneHostViolation) {
  std::string url1 = "http://foo.com";
  std::string url2 = "http://foo.com/bar";

  AddTestResource(url1, "text/css");
  AddTestResource(url2, "text/css");

  std::vector<std::string> no_violations;

  std::vector<std::string> css_violations;
  css_violations.push_back(url1);
  css_violations.push_back(url2);

  CheckViolations(pagespeed::CSS, 1, 1, css_violations);
  CheckViolations(pagespeed::JS, 0, 0, no_violations);
}

TEST_F(MinimizeResourcesTest, TwoCssResourcesFromTwoHostsNoViolation) {
  std::string url1 = "http://foo.com";
  std::string url2 = "http://bar.com";

  AddTestResource(url1, "text/css");
  AddTestResource(url2, "text/css");

  std::vector<std::string> no_violations;

  CheckViolations(pagespeed::CSS, 0, 0, no_violations);
  CheckViolations(pagespeed::JS, 0, 0, no_violations);
}

TEST_F(MinimizeResourcesTest, FourCssResourcesFromTwoHostsViolation) {
  std::string url1 = "http://a.com";
  std::string url2 = "http://a.com/foo";
  std::string url3 = "http://b.com";
  std::string url4 = "http://b.com/foo";

  AddTestResource(url1, "text/css");
  AddTestResource(url2, "text/css");
  AddTestResource(url3, "text/css");
  AddTestResource(url4, "text/css");

  std::vector<std::string> no_violations;

  std::vector<std::string> css_violations;
  css_violations.push_back(url1);
  css_violations.push_back(url2);
  css_violations.push_back(url3);
  css_violations.push_back(url4);

  CheckViolations(pagespeed::CSS, 2, 2, css_violations);
  CheckViolations(pagespeed::JS, 0, 0, no_violations);
}

TEST_F(MinimizeResourcesTest, ThreeCssResourcesFromOneHostViolation) {
  std::string url1 = "http://foo.com";
  std::string url2 = "http://foo.com/bar";
  std::string url3 = "http://foo.com/baz";

  AddTestResource(url1, "text/css");
  AddTestResource(url2, "text/css");
  AddTestResource(url3, "text/css");

  std::vector<std::string> no_violations;
  std::vector<std::string> css_violations;
  css_violations.push_back(url1);
  css_violations.push_back(url2);
  css_violations.push_back(url3);

  CheckViolations(pagespeed::CSS, 2, 1, css_violations);
  CheckViolations(pagespeed::JS, 0, 0, no_violations);
}

TEST_F(MinimizeResourcesTest, TwoJsResourcesFromOneHostViolation) {
  std::string url1 = "http://foo.com";
  std::string url2 = "http://foo.com/bar";

  AddTestResource(url1, "application/x-javascript");
  AddTestResource(url2, "application/x-javascript");

  std::vector<std::string> no_violations;
  std::vector<std::string> js_violations;
  js_violations.push_back(url1);
  js_violations.push_back(url2);

  CheckViolations(pagespeed::CSS, 0, 0, no_violations);
  CheckViolations(pagespeed::JS, 1, 1, js_violations);
}

}  // namespace
