// Copyright 2010 Google Inc. All Rights Reserved.
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

#include "pagespeed/core/resource.h"
#include "pagespeed/core/resource_collection.h"
#include "pagespeed/core/resource_filter.h"
#include "pagespeed/testing/pagespeed_test.h"

namespace {

using pagespeed::PagespeedInput;
using pagespeed::RedirectRegistry;
using pagespeed::Resource;
using pagespeed::ResourceCollection;
using pagespeed::ResourceVector;

static const char* kURL1 = "http://www.foo.com/";
static const char* kURL2 = "http://www.bar.com/";
static const char* kURL3 = "http://www.baz.com/";
static const char* kURL4 = "http://www.zzz.com/";
static const char* kNonCanonUrl = "http://example.com";
static const char* kCanonicalizedUrl = "http://example.com/";
static const char* kNonCanonUrlFragment = "http://example.com#foo";

Resource* NewResource(const std::string& url, int status_code) {
  Resource* resource = new Resource;
  resource->SetRequestUrl(url);
  resource->SetResponseStatusCode(status_code);
  return resource;
}

Resource* New200Resource(const std::string& url) {
  return NewResource(url, 200);
}

class Violation {
 public:
  Violation(int _expected_request_savings,
            const std::vector<std::string>& _urls)
      : expected_request_savings(_expected_request_savings),
        urls(_urls) {
  }

  int expected_request_savings;
  std::vector<std::string> urls;
};

class RedirectRegistryTest : public ::pagespeed_testing::PagespeedTest {
 protected:
  void AddResourceUrl(const std::string& url, int status_code) {
    Resource* resource = new Resource;
    resource->SetRequestUrl(url);
    resource->SetRequestMethod("GET");
    resource->SetResponseStatusCode(status_code);
    AddResource(resource);
  }

  void AddRedirect(const std::string& url, const std::string& location) {
    Resource* resource = new Resource;
    resource->SetRequestUrl(url);
    resource->SetRequestMethod("GET");
    resource->SetResponseStatusCode(302);
    if (!location.empty()) {
      resource->AddResponseHeader("Location", location);
    }
    AddResource(resource);
  }

  void CheckViolations(const std::vector<Violation>& expected_violations) {
    const RedirectRegistry::RedirectChainVector& redirect_chains =
        pagespeed_input()->GetResourceCollection()
        .GetRedirectRegistry()->GetRedirectChains();

    ASSERT_EQ(redirect_chains.size(), expected_violations.size());
    for (size_t idx = 0; idx < redirect_chains.size(); idx++) {
      const RedirectRegistry::RedirectChain& chain = redirect_chains[idx];
      const Violation& violation = expected_violations[idx];

      ASSERT_EQ(violation.urls.size(), chain.size());

      for (size_t url_idx = 0;
           url_idx < chain.size();
           ++url_idx) {
        EXPECT_EQ(violation.urls[url_idx], chain[url_idx]->GetRequestUrl())
            << "At index: " << url_idx;
      }
    }
  }
};

TEST_F(RedirectRegistryTest, SimpleRedirect) {
  std::string url1 = "http://foo.com/";
  std::string url2 = "http://www.foo.com/";

  AddRedirect(url1, url2);
  AddResourceUrl(url2, 200);
  Freeze();

  std::vector<std::string> urls;
  urls.push_back(url1);
  urls.push_back(url2);

  std::vector<Violation> violations;
  violations.push_back(Violation(1, urls));

  CheckViolations(violations);
}

TEST_F(RedirectRegistryTest, RedirectChain) {
  std::string url1 = "http://foo.com/";
  std::string url2 = "http://www.foo.com/";
  std::string url3 = "http://www.foo.com/index.html";

  AddRedirect(url1, url2);
  AddRedirect(url2, url3);
  AddResourceUrl(url3, 200);
  Freeze();

  std::vector<std::string> urls;
  urls.push_back(url1);
  urls.push_back(url2);
  urls.push_back(url3);

  std::vector<Violation> violations;
  violations.push_back(Violation(1, urls));

  CheckViolations(violations);
}

TEST_F(RedirectRegistryTest, NoRedirect) {
  std::string url1 = "http://foo.com/";
  std::string url2 = "http://www.foo.com/";

  AddResourceUrl(url1, 200);
  AddResourceUrl(url2, 200);
  Freeze();

  std::vector<Violation> violations;
  CheckViolations(violations);
}

TEST_F(RedirectRegistryTest, MissingDestination) {
  std::string url1 = "http://foo.com/";
  std::string url2 = "http://www.foo.com/";
  AddRedirect(url1, url2);
  Freeze();

  std::vector<std::string> urls;
  urls.push_back(url1);

  std::vector<Violation> violations;
  violations.push_back(Violation(1, urls));

  CheckViolations(violations);
}

TEST_F(RedirectRegistryTest, FinalRedirectTarget) {
  std::string url1 = "http://foo.com/";
  std::string url2 = "http://www.foo.com/";
  std::string url3 = "http://www.foo.com/index.html";

  AddRedirect(url1, url2);
  AddRedirect(url2, url3);
  AddResourceUrl(url3, 200);
  Freeze();

  const PagespeedInput* input = pagespeed_input();
  const RedirectRegistry& redirect_registry =
      *input->GetResourceCollection().GetRedirectRegistry();

  const Resource* resource1 = input->GetResourceWithUrlOrNull(url1);
  ASSERT_TRUE(NULL != resource1);
  const Resource* resource2 = input->GetResourceWithUrlOrNull(url2);
  ASSERT_TRUE(NULL != resource2);
  const Resource* resource3 = input->GetResourceWithUrlOrNull(url3);
  ASSERT_TRUE(NULL != resource3);
  EXPECT_EQ(resource3, redirect_registry.GetFinalRedirectTarget(resource1));
  EXPECT_EQ(resource3, redirect_registry.GetFinalRedirectTarget(resource2));
  EXPECT_EQ(resource3, redirect_registry.GetFinalRedirectTarget(resource3));
  EXPECT_EQ(NULL, redirect_registry.GetFinalRedirectTarget(NULL));
}

TEST(ResourceCollectionTest, DisallowDuplicates) {
  ResourceCollection coll;

  EXPECT_TRUE(coll.AddResource(NewResource(kURL1, 200)));
  EXPECT_TRUE(coll.AddResource(NewResource(kURL2, 200)));
  EXPECT_FALSE(coll.AddResource(NewResource(kURL2, 200)));
  ASSERT_TRUE(coll.Freeze());
  ASSERT_EQ(coll.num_resources(), 2);
  EXPECT_EQ(coll.GetResource(0).GetRequestUrl(), kURL1);
  EXPECT_EQ(coll.GetResource(1).GetRequestUrl(), kURL2);
}

TEST(ResourceCollectionTest, GetMutableResource) {
  ResourceCollection coll;

  EXPECT_TRUE(coll.AddResource(NewResource(kURL1, 200)));
  EXPECT_TRUE(coll.AddResource(NewResource(kURL2, 200)));
  EXPECT_FALSE(coll.AddResource(NewResource(kURL2, 200)));
  ASSERT_EQ(coll.num_resources(), 2);
  EXPECT_EQ(coll.GetMutableResource(0)->GetRequestUrl(), kURL1);
  EXPECT_EQ(coll.GetMutableResource(1)->GetRequestUrl(), kURL2);
  EXPECT_EQ(coll.GetMutableResourceWithUrlOrNull(kURL1)->GetRequestUrl(),
            kURL1);
  EXPECT_EQ(coll.GetMutableResourceWithUrlOrNull(kURL2)->GetRequestUrl(),
            kURL2);

  ASSERT_TRUE(coll.Freeze());
#ifdef NDEBUG
  ASSERT_EQ(NULL, coll.GetMutableResource(0));
#else
  ASSERT_DEATH(coll.GetMutableResource(0),
               "Unable to get mutable resource after freezing.");
#endif
}

TEST(ResourceCollectionTest, FilterBadResources) {
  ResourceCollection coll;
  EXPECT_FALSE(coll.AddResource(NewResource("", 0)));
  EXPECT_FALSE(coll.AddResource(NewResource("", 200)));
  EXPECT_FALSE(coll.AddResource(NewResource(kURL1, 0)));
  EXPECT_FALSE(coll.AddResource(NewResource(kURL1, -1)));
  ASSERT_TRUE(coll.Freeze());
}

TEST(ResourceCollectionTest, FilterResources) {
  ResourceCollection coll(
      new pagespeed::NotResourceFilter(new pagespeed::AllowAllResourceFilter));
  EXPECT_FALSE(coll.AddResource(NewResource(kURL1, 200)));
  ASSERT_TRUE(coll.Freeze());
}

// Make sure SetPrimaryResourceUrl canonicalizes its coll.
TEST(ResourceCollectionTest, GetResourceWithUrlOrNull) {
  ResourceCollection coll;
  EXPECT_TRUE(coll.AddResource(NewResource(kNonCanonUrl, 200)));
  ASSERT_TRUE(coll.Freeze());

  const Resource* r1 = coll.GetResourceWithUrlOrNull(kNonCanonUrl);
  const Resource* r2 = coll.GetResourceWithUrlOrNull(kCanonicalizedUrl);
  ASSERT_TRUE(coll.has_resource_with_url(kNonCanonUrlFragment));
  ASSERT_TRUE(r1 != NULL);
  ASSERT_TRUE(r2 != NULL);
  ASSERT_EQ(r1, r2);
  ASSERT_NE(kNonCanonUrl, r1->GetRequestUrl());
  ASSERT_EQ(kCanonicalizedUrl, r1->GetRequestUrl());
  ASSERT_NE(kNonCanonUrl, r2->GetRequestUrl());
  ASSERT_EQ(kCanonicalizedUrl, r2->GetRequestUrl());
}

TEST(ResourcesInRequestOrderTest, NoResourcesWithStartTimes) {
  ResourceCollection coll;
  coll.AddResource(New200Resource(kURL1));
  coll.AddResource(New200Resource(kURL2));
  coll.Freeze();
  ASSERT_EQ(NULL, coll.GetResourcesInRequestOrder());
}

TEST(ResourcesInRequestOrderTest, SomeResourcesWithStartTimes) {
  ResourceCollection coll;

  {
    Resource* r = New200Resource(kURL1);
    r->SetRequestStartTimeMillis(0);
    coll.AddResource(r);
  }
  {
    Resource* r = New200Resource(kURL2);
    r->SetRequestStartTimeMillis(1);
    coll.AddResource(r);
  }
  coll.AddResource(New200Resource(kURL3));
  coll.Freeze();
  ASSERT_EQ(NULL, coll.GetResourcesInRequestOrder());
}

TEST(ResourcesInRequestOrderTest, ResourcesWithStartTimes) {
  ResourceCollection coll;

  // We intentionally use the same time for two resources here, to
  // make sure we don't accidentally filter out duplicates (e.g. if we
  // used a set<>). ResourceCollection uses stable_sort so we should
  // expect the sort order to be stable even with duplicate values.

  {
    Resource* r = New200Resource(kURL4);
    r->SetRequestStartTimeMillis(0);
    coll.AddResource(r);
  }
  {
    Resource* r = New200Resource(kURL3);
    r->SetRequestStartTimeMillis(2);
    coll.AddResource(r);
  }
  {
    Resource* r = New200Resource(kURL1);
    r->SetRequestStartTimeMillis(2);
    coll.AddResource(r);
  }
  {
    Resource* r = New200Resource(kURL2);
    r->SetRequestStartTimeMillis(1);
    coll.AddResource(r);
  }
  coll.Freeze();
  const ResourceVector& rv(*coll.GetResourcesInRequestOrder());
  ASSERT_EQ(4U, rv.size());
  ASSERT_EQ(kURL4, rv[0]->GetRequestUrl());
  ASSERT_EQ(kURL2, rv[1]->GetRequestUrl());
  ASSERT_EQ(kURL3, rv[2]->GetRequestUrl());
  ASSERT_EQ(kURL1, rv[3]->GetRequestUrl());
}

}  // namespace
