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
#include "pagespeed/rules/specify_a_cache_validator.h"
#include "testing/gtest/include/gtest/gtest.h"

using pagespeed::rules::SpecifyACacheValidator;
using pagespeed::PagespeedInput;
using pagespeed::Resource;
using pagespeed::Result;
using pagespeed::Results;
using pagespeed::ResultProvider;
using pagespeed::Savings;

namespace {

class SpecifyACacheValidatorTest : public ::testing::Test {
 protected:
  virtual void SetUp() {
    input_.reset(new PagespeedInput);
  }

  virtual void TearDown() {
    input_.reset();
  }

  void AddTestResource(const char* url,
                       const char* last_modified_header) {
    Resource* resource = new Resource;
    resource->SetRequestUrl(url);
    resource->SetRequestMethod("GET");
    resource->SetResponseStatusCode(200);
    resource->AddResponseHeader("Content-Type", "image/png");
    if (last_modified_header != NULL) {
      resource->AddResponseHeader("Last-Modified", last_modified_header);
    }
    input_->AddResource(resource);
  }

  void CheckNoViolations() {
    SpecifyACacheValidator rule;

    Results results;
    ResultProvider provider(rule, &results);
    ASSERT_TRUE(rule.AppendResults(*input_, &provider));
    ASSERT_EQ(0, results.results_size());
  }

  void CheckOneViolation(const char *url) {
    SpecifyACacheValidator rule;

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

TEST_F(SpecifyACacheValidatorTest, MissingCacheValidator) {
  AddTestResource("http://www.example.com/", NULL);
  ASSERT_EQ(1, input_->num_resources());
  CheckOneViolation("http://www.example.com/");
}

TEST_F(SpecifyACacheValidatorTest, HasCacheValidator) {
  AddTestResource("http://www.example.com/1",
                  "Thu, 18 Mar 2010 10:36:52 EDT");
  ASSERT_EQ(1, input_->num_resources());
  CheckNoViolations();
}

TEST_F(SpecifyACacheValidatorTest, InvalidCacheValidator) {
  AddTestResource("http://www.example.com/1", "0");
  ASSERT_EQ(1, input_->num_resources());
  CheckOneViolation("http://www.example.com/1");
}

TEST_F(SpecifyACacheValidatorTest, ExplicitNoCacheDirective) {
  Resource* resource = new Resource;
  resource->SetRequestUrl("http://www.example.com/");
  resource->SetRequestMethod("GET");
  resource->AddResponseHeader("Content-Type", "image/png");
  resource->SetResponseStatusCode(200);
  input_->AddResource(resource);

  // This resource should cause a violation.
  CheckOneViolation("http://www.example.com/");

  // Now add a no-cache directive. We expect the resource to no longer
  // cause a violation.
  resource->AddResponseHeader("Pragma", "no-cache");
  CheckNoViolations();
}

}  // namespace
