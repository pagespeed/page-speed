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

#include "base/memory/scoped_ptr.h"
#include "pagespeed/core/pagespeed_input.h"
#include "pagespeed/core/resource.h"
#include "pagespeed/core/result_provider.h"
#include "pagespeed/proto/pagespeed_output.pb.h"
#include "pagespeed/rules/avoid_bad_requests.h"
#include "pagespeed/testing/pagespeed_test.h"

using pagespeed::rules::AvoidBadRequests;
using pagespeed::PagespeedInput;
using pagespeed::Resource;
using pagespeed::Result;
using pagespeed::Results;
using pagespeed::ResultProvider;
using ::pagespeed_testing::PagespeedRuleTest;

namespace {

class AvoidBadRequestsTest : public PagespeedRuleTest<AvoidBadRequests> {
 protected:
  void AddTestResource(const std::string &url,
                       const int status_code,
                       const std::string &body) {
    Resource* resource = new Resource;
    resource->SetRequestUrl(url);
    resource->SetRequestMethod("GET");
    resource->SetResponseStatusCode(status_code);
    resource->SetResponseBody(body);
    AddResource(resource);
  }

  void CheckNoViolations() {
    ASSERT_TRUE(AppendResults());
    ASSERT_EQ(num_results(), 0);
  }

  void CheckOneViolation(const std::string &url) {
    ASSERT_TRUE(AppendResults());
    ASSERT_EQ(num_results(), 1);
    const Result& result = this->result(0);
    ASSERT_EQ(result.savings().requests_saved(), 1);
    ASSERT_EQ(result.resource_urls_size(), 1);
    ASSERT_EQ(result.resource_urls(0), url);
  }
};

TEST_F(AvoidBadRequestsTest, NoProblems) {
  AddTestResource("http://www.example.com/hello.txt",
                  200, "Hello, world!");
  AddTestResource("http://www.example.com/goodbye.txt",
                  200, "Goodbye, world!");
  Freeze();
  CheckNoViolations();
}

TEST_F(AvoidBadRequestsTest, MissingImage) {
  AddTestResource("http://www.example.com/hello.txt",
                  200, "Hello, world!");
  AddTestResource("http://www.example.com/missing.png",
                  404, "");
  AddTestResource("http://www.example.com/goodbye.txt",
                  200, "Goodbye, world!");
  Freeze();
  CheckOneViolation("http://www.example.com/missing.png");
}

TEST_F(AvoidBadRequestsTest, PrimaryResourceIsExcluded) {
  Resource* primary = NewPrimaryResource("http://www.example.com/");
  primary->SetResponseStatusCode(404);
  Freeze();
  CheckNoViolations();
}

}  // namespace
