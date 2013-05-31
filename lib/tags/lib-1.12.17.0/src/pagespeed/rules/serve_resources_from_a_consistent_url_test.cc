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

#include <algorithm>
#include <string>
#include <vector>

#include "base/memory/scoped_ptr.h"
#include "pagespeed/core/pagespeed_input.h"
#include "pagespeed/core/resource.h"
#include "pagespeed/core/result_provider.h"
#include "pagespeed/proto/pagespeed_output.pb.h"
#include "pagespeed/rules/serve_resources_from_a_consistent_url.h"
#include "pagespeed/testing/pagespeed_test.h"

using pagespeed::rules::ServeResourcesFromAConsistentUrl;
using pagespeed::PagespeedInput;
using pagespeed::Resource;
using pagespeed::Result;
using pagespeed::Results;
using pagespeed::ResultProvider;
using pagespeed_testing::PagespeedRuleTest;

namespace {

const char *kResponseBodies[] = {
  "first response body"
  "                                                  "
  "                                                  ",
  "second response body"
  "                                                  "
  "                                                  ",
  "third response body"
  "                                                  "
  "                                                  ",
};

const char *kResponseUrls[2][3] = {
  {
    "http://www.example.com/bac",
    "http://www.example.com/abracadabra",
    "http://www.example.com/c",
  },
  {
    "http://www.foo.com/z",
    "http://www.foo.com/yy",
    "http://www.foo.com/abc",
  }
};

class ServeResourcesFromAConsistentUrlTest
    : public PagespeedRuleTest<ServeResourcesFromAConsistentUrl> {
 protected:
  pagespeed::Resource* AddTestResource(const char* url,
                                       const std::string& body,
                                       int response_code = 200) {
    Resource* resource = new Resource;
    resource->SetRequestUrl(url);
    resource->SetRequestMethod("GET");
    resource->SetResponseStatusCode(response_code);
    if (response_code == 200) {
      resource->SetResourceType(pagespeed::HTML);
    }
    resource->SetResponseBody(body);

    AddResource(resource);
    return resource;
  }

  int ComputeSavings(size_t num_resources, const char *body) {
    return (num_resources - 1) * strlen(body);
  }

  void CheckViolation(size_t num_collisions, size_t num_resources) {
    Freeze();
    ASSERT_TRUE(AppendResults());
    ASSERT_EQ(num_collisions, static_cast<size_t>(num_results()));
    for (int result_idx = 0; result_idx < num_results(); result_idx++) {
      const Result& res = result(result_idx);

      int expected_savings =
          ComputeSavings(num_resources, kResponseBodies[result_idx]);
      ASSERT_EQ(num_resources - 1,
                static_cast<size_t>(res.savings().requests_saved()));
      ASSERT_EQ(expected_savings, res.savings().response_bytes_saved());
      ASSERT_EQ(num_resources,
                static_cast<size_t>(res.resource_urls_size()));

      // Now verify that the list or resource URLs in the Result
      // contains the expected contents. We sort both lists, then
      // assert that the sorted results are equal.
      std::vector<std::string> expected_urls;
      std::vector<std::string> actual_urls;
      for (size_t url_idx = 0; url_idx < num_resources; url_idx++) {
        expected_urls.push_back(kResponseUrls[result_idx][url_idx]);
        actual_urls.push_back(res.resource_urls(url_idx));
      }
      std::sort(expected_urls.begin(), expected_urls.end());
      std::sort(actual_urls.begin(), actual_urls.end());

      ASSERT_TRUE(expected_urls == actual_urls);
    }
  }
};

TEST_F(ServeResourcesFromAConsistentUrlTest, NoResources) {
  CheckNoViolations();
}

TEST_F(ServeResourcesFromAConsistentUrlTest, SingleResource) {
  AddTestResource("http://www.example.com", kResponseBodies[0]);
  CheckNoViolations();
}

TEST_F(ServeResourcesFromAConsistentUrlTest, SingleEmptyResource) {
  AddTestResource("http://www.example.com", "");
  CheckNoViolations();
}

TEST_F(ServeResourcesFromAConsistentUrlTest, MultipleEmptyResources) {
  AddTestResource(kResponseUrls[0][0], "");
  AddTestResource(kResponseUrls[0][1], "");
  AddTestResource(kResponseUrls[0][2], "");
  CheckNoViolations();
}

TEST_F(ServeResourcesFromAConsistentUrlTest, IgnoreRedirects) {
  AddTestResource(kResponseUrls[0][0], kResponseBodies[0], 301);
  AddTestResource(kResponseUrls[0][1], kResponseBodies[0], 301);
  AddTestResource(kResponseUrls[0][2], kResponseBodies[0], 301);
  CheckNoViolations();
}

TEST_F(ServeResourcesFromAConsistentUrlTest, DifferentResources) {
  AddTestResource(kResponseUrls[0][0], kResponseBodies[0]);
  AddTestResource(kResponseUrls[0][1], kResponseBodies[1]);
  AddTestResource(kResponseUrls[0][2], kResponseBodies[2]);
  CheckNoViolations();
}

TEST_F(ServeResourcesFromAConsistentUrlTest, SameResourceTwoUrls0) {
  AddTestResource(kResponseUrls[0][0], kResponseBodies[0]);
  AddTestResource(kResponseUrls[0][1], kResponseBodies[0]);
  CheckViolation(1, 2);
}

TEST_F(ServeResourcesFromAConsistentUrlTest, SameResourceTwoUrls1) {
  pagespeed::Resource* r1 =
      AddTestResource(kResponseUrls[0][0], kResponseBodies[0]);
  pagespeed::Resource* r2 =
      AddTestResource(kResponseUrls[0][1], kResponseBodies[0]);
  // Now change the response codes of both resources, and verify that
  // they no longer trigger a violation.
  r1->SetResponseStatusCode(500);
  r2->SetResponseStatusCode(500);
  CheckNoViolations();
}

TEST_F(ServeResourcesFromAConsistentUrlTest, SameResourceTwoUrls2) {
  AddTestResource(kResponseUrls[0][0], kResponseBodies[0]);
  AddTestResource(kResponseUrls[0][1], kResponseBodies[0]);
  AddTestResource(kResponseUrls[0][2], "");
  CheckViolation(1, 2);
}

TEST_F(ServeResourcesFromAConsistentUrlTest, SameResourceTwoUrls3) {
  AddTestResource(kResponseUrls[0][0], kResponseBodies[0]);
  AddTestResource(kResponseUrls[0][1], kResponseBodies[0]);
  AddTestResource(kResponseUrls[0][2], kResponseBodies[1]);
  CheckViolation(1, 2);
}

TEST_F(ServeResourcesFromAConsistentUrlTest, SameResourceThreeUrls) {
  AddTestResource(kResponseUrls[0][0], kResponseBodies[0]);
  AddTestResource(kResponseUrls[0][1], kResponseBodies[0]);
  AddTestResource(kResponseUrls[0][2], kResponseBodies[0]);
  CheckViolation(1, 3);
}

TEST_F(ServeResourcesFromAConsistentUrlTest, TwoDuplicatedResources) {
  AddTestResource(kResponseUrls[0][0], kResponseBodies[0]);
  AddTestResource(kResponseUrls[0][1], kResponseBodies[0]);
  AddTestResource(kResponseUrls[1][0], kResponseBodies[1]);
  AddTestResource(kResponseUrls[1][1], kResponseBodies[1]);
  CheckViolation(2, 2);
}

TEST_F(ServeResourcesFromAConsistentUrlTest, BinaryResponseBodies) {
  std::string body_a("abcdefghij");
  std::string body_b("abcde");
  body_a[5] = '\0';
  AddTestResource("http://www.example.com/a", body_a);
  AddTestResource("http://www.example.com/b", body_b);
  CheckNoViolations();
}

TEST_F(ServeResourcesFromAConsistentUrlTest, CrossDomainXml) {
  const char* kCrossDomainBody = "example response body";
  AddTestResource("http://www.example.com/crossdomain.xml", kCrossDomainBody);
  AddTestResource("http://foo.example.com/crossdomain.xml", kCrossDomainBody);
  CheckNoViolations();
}

TEST_F(ServeResourcesFromAConsistentUrlTest, SkipTrackingPixels) {
  // Any non-zero-length body is sufficient.
  static const char* kBody = "a";
  pagespeed::Resource* a = NewPngResource(kResponseUrls[0][0]);
  pagespeed::Resource* b = NewPngResource(kResponseUrls[0][1]);
  a->SetResponseBody(kBody);
  b->SetResponseBody(kBody);
  a->AddResponseHeader("Cache-Control", "no-cache");
  b->AddResponseHeader("Cache-Control", "no-cache");
  pagespeed_testing::FakeImageAttributesFactory::ResourceSizeMap size_map;
  size_map[a] = std::make_pair(1, 1);
  size_map[b] = std::make_pair(1, 1);
  AddFakeImageAttributesFactory(size_map);
  CheckNoViolations();
}

}  // namespace
