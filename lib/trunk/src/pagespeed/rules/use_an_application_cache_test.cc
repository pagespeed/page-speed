// Copyright 2011 Google Inc. All Rights Reserved.
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

#include "pagespeed/core/result_provider.h"
#include "pagespeed/proto/pagespeed_output.pb.h"
#include "pagespeed/rules/use_an_application_cache.h"
#include "pagespeed/testing/pagespeed_test.h"

using pagespeed::Result;
using pagespeed::rules::UseAnApplicationCache;
using pagespeed::ResultProvider;
using pagespeed::RuleResults;

namespace {

const char* kHtmlWithManifest =
    "<!DOCTYPE HTML PUBLIC \"-//IETF//DTD HTML 2.0//EN\">"
    "<html manifest=\"html.appcache\">"
    "<body>Hello, world."
    "</body></html>";

const char* kHtmlWithMixCaseManifest =
    "<!DOCTYPE HTML PUBLIC \"-//IETF//DTD HTML 2.0//EN\">"
    "<html ManiFest=\"html.appcache\">"
    "<body>Hello, world."
    "</body></html>";

const char* kHtmlWithoutManifest =
    "<!DOCTYPE HTML PUBLIC \"-//IETF//DTD HTML 2.0//EN\">"
    "<html>"
    "<body>Hello, world."
    "</body></html>";

const char* kRootUrl = "http://www.example.com/";

class UseAnApplicationCacheTest : public
    ::pagespeed_testing::PagespeedRuleTest<UseAnApplicationCache> {
 protected:
  virtual void DoSetUp() {
    NewPrimaryResource(kRootUrl);
  }

  void CheckFormattedOutput(const std::string& expected_output) {
    ASSERT_TRUE(AppendResults());
    EXPECT_EQ(expected_output, FormatResults());
  }

  void CheckExpectedViolations(const std::vector<std::string>& expected) {
    ASSERT_TRUE(AppendResults());
    ASSERT_EQ(expected.size(), static_cast<size_t>(num_results()));

    for (size_t idx = 0; idx < expected.size(); ++idx) {
      ASSERT_EQ(expected.size(),
                static_cast<size_t>(result(idx).resource_urls_size()));
      for (size_t jdx = 0; jdx < expected.size(); ++jdx) {
        ASSERT_EQ(1, result(idx).resource_urls_size());
        ASSERT_EQ(expected[idx], result(idx).resource_urls(0));
      }
    }

    if (!expected.empty()) {
      // If there are violations, make sure we have a non-zero impact.
      ASSERT_GT(ComputeRuleImpact(), 0.0);
    }
  }
};

TEST_F(UseAnApplicationCacheTest, EmptyDom) {
  Freeze();
  std::vector<std::string> no_resources;
  CheckExpectedViolations(no_resources);
}

TEST_F(UseAnApplicationCacheTest, NoUseAnApplicationCache) {
  primary_resource()->SetResponseBody(kHtmlWithoutManifest);

  Freeze();
  std::vector<std::string> urls;
  urls.push_back(kRootUrl);
  CheckExpectedViolations(urls);
}

TEST_F(UseAnApplicationCacheTest, UseAnApplicationCache) {
  primary_resource()->SetResponseBody(kHtmlWithManifest);

  Freeze();
  std::vector<std::string> empty;
  CheckExpectedViolations(empty);
}

TEST_F(UseAnApplicationCacheTest, UseAnApplicationCacheMixCase) {
  primary_resource()->SetResponseBody(kHtmlWithMixCaseManifest);

  Freeze();
  std::vector<std::string> empty;
  CheckExpectedViolations(empty);
}


TEST_F(UseAnApplicationCacheTest, FormatTest) {
  std::string expected =
      "Using an application cache allows a page to show up immediately. The "
      "following HTML documents can use an application cache to reduce the "
      "time it takes for users to be able to interact with the page:\n"
      "  http://www.example.com/\n";
  primary_resource()->SetResponseBody(kHtmlWithoutManifest);

  Freeze();
  CheckFormattedOutput(expected);
}

}  // namespace
