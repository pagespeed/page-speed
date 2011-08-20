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

#include "base/scoped_ptr.h"
#include "pagespeed/core/pagespeed_input.h"
#include "pagespeed/core/resource.h"
#include "pagespeed/core/result_provider.h"
#include "pagespeed/proto/pagespeed_output.pb.h"
#include "pagespeed/rules/remove_query_strings_from_static_resources.h"
#include "pagespeed/testing/pagespeed_test.h"

using pagespeed::rules::RemoveQueryStringsFromStaticResources;
using pagespeed::PagespeedInput;
using pagespeed::Resource;
using pagespeed::Result;
using pagespeed::Results;
using pagespeed::ResultProvider;
using pagespeed_testing::PagespeedRuleTest;

namespace {

class RemoveQueryStringsFromStaticResourcesTest
    : public PagespeedRuleTest<RemoveQueryStringsFromStaticResources> {
 protected:
  void AddTestResource(const std::string& url,
                       const std::string& type) {
    Resource* resource = new Resource;
    resource->SetRequestUrl(url);
    resource->SetRequestMethod("GET");
    resource->SetResponseStatusCode(200);
    resource->SetResponseBody("Hello, world!");
    resource->AddResponseHeader("Content-Type", type);
    resource->AddResponseHeader("Cache-Control", "public, max-age=1000000");
    AddResource(resource);
  }

  void CheckOneViolation(const std::string& url) {
    Freeze();
    ASSERT_TRUE(AppendResults());
    ASSERT_EQ(1, num_results());
    const Result& result0 = result(0);
    ASSERT_EQ(1, result0.resource_urls_size());
    ASSERT_EQ(url, result0.resource_urls(0));
  }
};

TEST_F(RemoveQueryStringsFromStaticResourcesTest, NoProblems) {
  AddTestResource("http://www.example.com/index.html",
                  "text/html");
  AddTestResource("http://static.example.com/image/40/30",
                  "image/png");
  CheckNoViolations();
}

TEST_F(RemoveQueryStringsFromStaticResourcesTest, OneViolation) {
  AddTestResource("http://www.example.com/index.html",
                  "text/html");
  AddTestResource("http://static.example.com/image?w=40&h=30",
                  "image/png");
  CheckOneViolation("http://static.example.com/image?w=40&h=30");
}

TEST_F(RemoveQueryStringsFromStaticResourcesTest, ExcludeNonStaticResources) {
  AddTestResource("http://www.example.com/index.html?query",
                  "text/html");
  CheckNoViolations();
  ASSERT_EQ(100, ComputeScore());
}

}  // namespace
