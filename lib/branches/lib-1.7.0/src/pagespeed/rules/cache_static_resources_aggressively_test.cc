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

#include <string>

#include "base/scoped_ptr.h"
#include "pagespeed/core/pagespeed_input.h"
#include "pagespeed/core/resource.h"
#include "pagespeed/core/result_provider.h"
#include "pagespeed/proto/pagespeed_output.pb.h"
#include "pagespeed/rules/cache_static_resources_aggressively.h"
#include "testing/gtest/include/gtest/gtest.h"

using pagespeed::rules::CacheStaticResourcesAggressively;
using pagespeed::CachingDetails;
using pagespeed::PagespeedInput;
using pagespeed::Resource;
using pagespeed::Result;
using pagespeed::ResultDetails;
using pagespeed::ResultVector;
using pagespeed::Results;
using pagespeed::ResultProvider;
using pagespeed::Savings;

namespace {

class CacheStaticResourcesAggressivelyTest : public ::testing::Test {
 protected:
  virtual void SetUp() {
    input_.reset(new PagespeedInput);
  }

  virtual void TearDown() {
    input_.reset();
  }

  void AddTestResource(const char* url,
                       const char* cache_control_header) {
    Resource* resource = new Resource;
    resource->SetRequestUrl(url);
    resource->SetRequestMethod("GET");
    resource->SetResponseStatusCode(200);
    resource->AddResponseHeader("Content-Type", "image/png");

    if (cache_control_header != NULL) {
      resource->AddResponseHeader("Cache-Control", cache_control_header);
    }
    input_->AddResource(resource);
  }

  void CheckNoViolations() {
    CacheStaticResourcesAggressively rule;

    Results results;
    ResultProvider provider(rule, &results);
    ASSERT_TRUE(rule.AppendResults(*input_, &provider));
    ASSERT_EQ(0, results.results_size());
  }

  void CheckOneViolation(const char *url,
                         int64 freshness_lifetime_millis,
                         int score) {
    CacheStaticResourcesAggressively rule;

    Results results;
    ResultProvider provider(rule, &results);
    ASSERT_TRUE(rule.AppendResults(*input_, &provider));
    ASSERT_EQ(1, results.results_size());

    ResultVector result_vector;
    result_vector.push_back(&results.results(0));
    ASSERT_EQ(score, rule.ComputeScore(*input_->input_information(),
                                       result_vector));

    const Result& result0 = results.results(0);
    ASSERT_EQ(result0.resource_urls_size(), 1);
    ASSERT_EQ(result0.resource_urls(0), url);

    ASSERT_TRUE(result0.has_details());
    const ResultDetails& details = result0.details();
    ASSERT_TRUE(details.HasExtension(CachingDetails::message_set_extension));
    const CachingDetails& caching_details =
      details.GetExtension(CachingDetails::message_set_extension);

    ASSERT_EQ(freshness_lifetime_millis,
              caching_details.freshness_lifetime_millis());
  }

  scoped_ptr<PagespeedInput> input_;
};

TEST_F(CacheStaticResourcesAggressivelyTest, ShortFreshnessLifetime) {
  AddTestResource("http://www.example.com/", "max-age=500");
  ASSERT_EQ(1, input_->num_resources());
  CheckOneViolation("http://www.example.com/", 500000, 0);
}

TEST_F(CacheStaticResourcesAggressivelyTest, LongFreshnessLifetime) {
  AddTestResource("http://www.example.com/1", "max-age=31536000");
  ASSERT_EQ(1, input_->num_resources());
  CheckNoViolations();
}

TEST_F(CacheStaticResourcesAggressivelyTest, NotCacheable) {
  AddTestResource("http://www.example.com/1", "no-cache");
  ASSERT_EQ(1, input_->num_resources());
  CheckNoViolations();
}

TEST_F(CacheStaticResourcesAggressivelyTest, BadFreshnessLifetime) {
  AddTestResource("http://www.example.com/1", "max-age=foo");
  ASSERT_EQ(1, input_->num_resources());
  CheckNoViolations();
}

TEST_F(CacheStaticResourcesAggressivelyTest, OneShortOneLongLifetime) {
  AddTestResource("http://www.example.com/a", "max-age=302400");
  AddTestResource("http://www.example.com/1", "max-age=31536000");
  ASSERT_EQ(2, input_->num_resources());
  CheckOneViolation("http://www.example.com/a", 302400000, 75);
}

TEST_F(CacheStaticResourcesAggressivelyTest, OneShortOneNoLifetime) {
  AddTestResource("http://www.example.com/a", "max-age=1");
  AddTestResource("http://www.example.com/1", NULL);
  ASSERT_EQ(2, input_->num_resources());
  CheckOneViolation("http://www.example.com/a", 1000, 0);
}

}  // namespace
