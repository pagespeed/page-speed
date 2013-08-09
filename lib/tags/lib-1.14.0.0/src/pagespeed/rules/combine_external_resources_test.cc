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
#include "pagespeed/proto/pagespeed_output.pb.h"
#include "pagespeed/rules/combine_external_resources.h"
#include "pagespeed/testing/pagespeed_test.h"

using pagespeed::rules::CombineExternalCss;
using pagespeed::rules::CombineExternalJavaScript;
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
  Violation(int _expected_rt_savings,
            const std::string& _host,
            const std::vector<std::string>& _urls)
      : expected_rt_savings(_expected_rt_savings),
        host(_host),
        urls(_urls) {
  }

  int expected_rt_savings;
  std::string host;
  std::vector<std::string> urls;
};

class CombineExternalResourcesTest : public ::pagespeed_testing::PagespeedTest {
 protected:
  Resource* AddTestResource(const std::string& url,
                            const std::string& content_type) {
    Resource* resource = new Resource;
    resource->SetRequestUrl(url);
    resource->AddResponseHeader("Content-Type", content_type);
    resource->SetRequestMethod("GET");
    resource->SetResponseStatusCode(200);
    AddResource(resource);
    return resource;
  }

  void CheckViolations(ResourceType type,
                       const std::vector<Violation>& expected_violations) {
    scoped_ptr<Rule> resource_rule;
    if (type == pagespeed::CSS) {
      resource_rule.reset(new CombineExternalCss());
    } else if (type == pagespeed::JS) {
      resource_rule.reset(new CombineExternalJavaScript());
    } else {
      CHECK(false);
    }

    RuleResults rule_results;
    ResultProvider provider(*resource_rule.get(), &rule_results, 0);
    const RuleInput rule_input(*pagespeed_input());
    resource_rule->AppendResults(rule_input, &provider);
    ASSERT_EQ(static_cast<size_t>(rule_results.results_size()),
              expected_violations.size());
    for (int idx = 0; idx < rule_results.results_size(); idx++) {
      const Result* result = &rule_results.results(idx);
      const Violation& violation = expected_violations[idx];

      ASSERT_EQ(result->savings().requests_saved(),
                violation.expected_rt_savings);

      ASSERT_EQ(static_cast<size_t>(result->resource_urls_size()),
                violation.urls.size());

      for (int url_idx = 0;
           url_idx < result->resource_urls_size();
           ++url_idx) {
        EXPECT_EQ(result->resource_urls(url_idx),
                  violation.urls[url_idx]);
      }
    }
  }
};

TEST_F(CombineExternalResourcesTest, OneUrlNoViolation) {
  std::string url = "http://foo.com/";

  AddTestResource(url, "text/css");

  std::vector<Violation> no_violations;

  Freeze();
  CheckViolations(pagespeed::JS, no_violations);
  CheckViolations(pagespeed::CSS, no_violations);
}

TEST_F(CombineExternalResourcesTest, OnePostOnloadOneNotNoViolation) {
  std::string url1 = "http://foo.com/";
  std::string url2 = "http://foo.com/bar";

  SetOnloadTimeMillis(10);

  AddTestResource(url1, "text/css");
  AddTestResource(url2, "text/css")->SetRequestStartTimeMillis(11);

  std::vector<Violation> no_violations;

  Freeze();
  CheckViolations(pagespeed::JS, no_violations);
  CheckViolations(pagespeed::CSS, no_violations);
}

TEST_F(CombineExternalResourcesTest, TwoCssResourcesFromOneHostNoViolation) {
  std::string url1 = "http://foo.com/";
  std::string url2 = "http://foo.com/bar";

  AddTestResource(url1, "text/css");
  AddTestResource(url2, "text/css");

  std::vector<Violation> no_violations;

  Freeze();
  CheckViolations(pagespeed::CSS, no_violations);
  CheckViolations(pagespeed::JS, no_violations);
}

TEST_F(CombineExternalResourcesTest, TwoCssResourcesFromTwoHostsNoViolation) {
  std::string url1 = "http://foo.com/";
  std::string url2 = "http://bar.com/";

  AddTestResource(url1, "text/css");
  AddTestResource(url2, "text/css");

  std::vector<Violation> no_violations;

  Freeze();
  CheckViolations(pagespeed::CSS, no_violations);
  CheckViolations(pagespeed::JS, no_violations);
}

TEST_F(CombineExternalResourcesTest, SixCssResourcesFromTwoHostsViolation) {
  std::string url1 = "http://a.com/";
  std::string url2 = "http://a.com/bar";
  std::string url3 = "http://a.com/foo";
  std::string url4 = "http://b.com/";
  std::string url5 = "http://b.com/bar";
  std::string url6 = "http://b.com/foo";

  AddTestResource(url1, "text/css");
  AddTestResource(url2, "text/css");
  AddTestResource(url3, "text/css");
  AddTestResource(url4, "text/css");
  AddTestResource(url5, "text/css");
  AddTestResource(url6, "text/css");

  std::vector<Violation> no_violations;

  std::vector<std::string> aUrls;
  aUrls.push_back(url1);
  aUrls.push_back(url2);
  aUrls.push_back(url3);

  std::vector<std::string> bUrls;
  bUrls.push_back(url4);
  bUrls.push_back(url5);
  bUrls.push_back(url6);

  std::vector<Violation> css_violations;
  css_violations.push_back(Violation(1, "a.com", aUrls));
  css_violations.push_back(Violation(1, "b.com", bUrls));

  Freeze();
  CheckViolations(pagespeed::CSS, css_violations);
  CheckViolations(pagespeed::JS, no_violations);
}

TEST_F(CombineExternalResourcesTest, ThreeCssResourcesFromOneHostViolation) {
  std::string url1 = "http://foo.com/";
  std::string url2 = "http://foo.com/bar";
  std::string url3 = "http://foo.com/baz";

  AddTestResource(url1, "text/css");
  AddTestResource(url2, "text/css");
  AddTestResource(url3, "text/css");

  std::vector<Violation> no_violations;

  std::vector<std::string> urls;
  urls.push_back(url1);
  urls.push_back(url2);
  urls.push_back(url3);

  std::vector<Violation> css_violations;
  css_violations.push_back(Violation(1, "foo.com", urls));

  Freeze();
  CheckViolations(pagespeed::CSS, css_violations);
  CheckViolations(pagespeed::JS, no_violations);
}

TEST_F(CombineExternalResourcesTest, TwoJsResourcesFromOneHostNoViolation) {
  std::string url1 = "http://foo.com/";
  std::string url2 = "http://foo.com/bar";

  AddTestResource(url1, "application/x-javascript");
  AddTestResource(url2, "application/x-javascript");

  std::vector<Violation> no_violations;

  Freeze();
  CheckViolations(pagespeed::CSS, no_violations);
  CheckViolations(pagespeed::JS, no_violations);
}

}  // namespace
