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
#include "pagespeed/rules/specify_a_cache_expiration.h"
#include "testing/gtest/include/gtest/gtest.h"

using pagespeed::rules::SpecifyACacheExpiration;
using pagespeed::PagespeedInput;
using pagespeed::Resource;
using pagespeed::Result;
using pagespeed::Results;
using pagespeed::ResultProvider;
using pagespeed::Savings;

namespace {

class SpecifyACacheExpirationTest : public ::testing::Test {
 protected:
  virtual void SetUp() {
    input_.reset(new PagespeedInput);
  }

  virtual void TearDown() {
    input_.reset();
  }

  void AddTestResource(const char* url,
                       int response_status_code,
                       const char* date_header,
                       const char *expires_header) {
    Resource* resource = new Resource;
    resource->SetRequestUrl(url);
    resource->SetRequestMethod("GET");

    resource->SetResponseStatusCode(response_status_code);
    if (date_header != NULL) {
      resource->AddResponseHeader("Date", date_header);
    }
    if (expires_header != NULL) {
      resource->AddResponseHeader("Expires", expires_header);
    }
    input_->AddResource(resource);
  }

  void CheckNoViolations() {
    SpecifyACacheExpiration rule;

    Results results;
    ResultProvider provider(rule, &results);
    ASSERT_TRUE(rule.AppendResults(*input_, &provider));
    ASSERT_EQ(0, results.results_size());
  }

  void CheckOneViolation(const char *url) {
    SpecifyACacheExpiration rule;

    Results results;
    ResultProvider provider(rule, &results);
    ASSERT_TRUE(rule.AppendResults(*input_, &provider));
    ASSERT_EQ(1, results.results_size());

    const Result& result0 = results.results(0);
    ASSERT_EQ(result0.resource_urls_size(), 1);
    ASSERT_EQ(result0.resource_urls(0), url);
  }

  scoped_ptr<PagespeedInput> input_;
};

TEST_F(SpecifyACacheExpirationTest, Required) {
  AddTestResource("http://www.example.com/",
                  200,
                  "Thu, 18 Mar 2010 10:36:52 EDT",
                  NULL);
  ASSERT_EQ(1, input_->num_resources());
  CheckOneViolation("http://www.example.com/");
}

TEST_F(SpecifyACacheExpirationTest, MissingDateHeader) {
  AddTestResource("http://www.example.com/", 200, NULL, NULL);
  ASSERT_EQ(1, input_->num_resources());
  CheckNoViolations();
}

TEST_F(SpecifyACacheExpirationTest, NotRequired) {
  AddTestResource("http://www.example.com/1",
                  500,
                  "Thu, 18 Mar 2010 10:36:52 EDT",
                  NULL);
  AddTestResource("http://www.example.com/2",
                  100,
                  "Thu, 18 Mar 2010 10:36:52 EDT",
                  NULL);
  ASSERT_EQ(2, input_->num_resources());
  CheckNoViolations();
}

TEST_F(SpecifyACacheExpirationTest, Same) {
  AddTestResource("http://www.example.com/",
                  200,
                  "Thu, 18 Mar 2010 10:36:52 EDT",
                  "Thu, 18 Mar 2010 10:36:52 EDT");
  ASSERT_EQ(1, input_->num_resources());
  CheckNoViolations();
}

TEST_F(SpecifyACacheExpirationTest, Past) {
  AddTestResource("http://www.example.com/",
                  200,
                  "Thu, 18 Mar 2010 10:36:52 EDT",
                  "Thu, 18 Mar 2010 10:36:51 EDT");
  ASSERT_EQ(1, input_->num_resources());
  CheckNoViolations();
}

TEST_F(SpecifyACacheExpirationTest, Future) {
  AddTestResource("http://www.example.com/",
                  200,
                  "Thu, 18 Mar 2010 10:36:52 EDT",
                  "Thu, 18 Mar 2010 10:36:53 EDT");
  ASSERT_EQ(1, input_->num_resources());
  CheckNoViolations();
}

TEST_F(SpecifyACacheExpirationTest, Invalid) {
  AddTestResource("http://www.example.com/",
                  200,
                  "Thu, 18 Mar 2010 10:36:52 EDT",
                  "0");
  ASSERT_EQ(1, input_->num_resources());

  // The RFC says that when an Expires header is not valid, it should
  // be treated as expired. Thus, the resource does have a cache
  // expiration and we should not warn about it.
  CheckNoViolations();
}

TEST_F(SpecifyACacheExpirationTest, SomeRequired) {
  AddTestResource("http://www.example.com/1",
                  100,
                  "Thu, 18 Mar 2010 10:36:52 EDT",
                  NULL);
  AddTestResource("http://www.example.com/2",
                  500,
                  "Thu, 18 Mar 2010 10:36:52 EDT",
                  NULL);
  AddTestResource("http://www.example.com/3",
                  200,
                  "Thu, 18 Mar 2010 10:36:52 EDT",
                  NULL);
  ASSERT_EQ(3, input_->num_resources());
  CheckOneViolation("http://www.example.com/3");
}

TEST_F(SpecifyACacheExpirationTest, NoDateHeader) {
  AddTestResource("http://www.example.com/",
                  200,
                  NULL,
                  NULL);
  ASSERT_EQ(1, input_->num_resources());

  // If the resource is generally cacheable but is missing a Date
  // header, it should not be included in the results.
  CheckNoViolations();
}

TEST_F(SpecifyACacheExpirationTest, MustRevalidate) {
  Resource* resource = new Resource;
  resource->SetRequestUrl("http://www.example.com/");
  resource->SetRequestMethod("GET");
  resource->SetResponseStatusCode(200);
  resource->AddResponseHeader("Date", "Thu, 18 Mar 2010 10:36:52 EDT");
  input_->AddResource(resource);
  ASSERT_EQ(1, input_->num_resources());

  CheckOneViolation("http://www.example.com/");

  // Now add a must-revalidate header. must-revalidate disables
  // heuristic caching, making a resource without a cache expiration
  // non-cacheable.
  resource->AddResponseHeader("Cache-Control", "must-revalidate");
  CheckNoViolations();
}

}  // namespace
