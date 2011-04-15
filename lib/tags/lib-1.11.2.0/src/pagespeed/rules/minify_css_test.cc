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
#include "pagespeed/rules/minify_css.h"
#include "pagespeed/testing/pagespeed_test.h"

using pagespeed::rules::MinifyCss;
using pagespeed::PagespeedInput;
using pagespeed::Resource;
using pagespeed::Result;
using pagespeed::Results;
using pagespeed::ResultProvider;
using pagespeed::ResultVector;
using pagespeed::RuleResults;

namespace {

// Unminified CSS.
const char* kUnminified = "body { color: red /*red*/; }";

// The same CSS, minified.
const char* kMinified = "body{color:red;}";

class MinifyCssTest : public ::pagespeed_testing::PagespeedTest {
 protected:
  void AddTestResource(const char* url,
                       const char* content_type,
                       const char* body) {
    Resource* resource = new Resource;
    resource->SetRequestUrl(url);
    resource->SetRequestMethod("GET");
    resource->SetResponseStatusCode(200);

    if (content_type != NULL) {
      resource->AddResponseHeader("Content-Type", content_type);
    }

    if (body != NULL) {
      resource->SetResponseBody(body);
    }
    AddResource(resource);
  }

  void CheckNoViolations() {
    CheckNoViolationsInternal(false);
    CheckNoViolationsInternal(true);
  }

  void CheckOneViolation(int score) {
    CheckOneViolationInternal(score, false);
    CheckOneViolationInternal(score, true);
  }

  void CheckError() {
    CheckErrorInternal(false);
    CheckErrorInternal(true);
  }

 private:
  void CheckNoViolationsInternal(bool save_optimized_content) {
    MinifyCss minify(save_optimized_content);

    RuleResults rule_results;
    ResultProvider provider(minify, &rule_results, 0);
    pagespeed::RuleInput rule_input(*pagespeed_input());
    ASSERT_TRUE(minify.AppendResults(rule_input, &provider));
    ASSERT_EQ(rule_results.results_size(), 0);
  }

  void CheckOneViolationInternal(int score, bool save_optimized_content) {
    MinifyCss minify(save_optimized_content);

    RuleResults rule_results;
    ResultProvider provider(minify, &rule_results, 0);
    pagespeed::RuleInput rule_input(*pagespeed_input());
    ASSERT_TRUE(minify.AppendResults(rule_input, &provider));
    ASSERT_EQ(rule_results.results_size(), 1);

    const Result& result = rule_results.results(0);

    if (save_optimized_content) {
      ASSERT_TRUE(result.has_optimized_content());
      EXPECT_EQ(kMinified, result.optimized_content());
    } else {
      ASSERT_FALSE(result.has_optimized_content());
    }

    ASSERT_EQ(static_cast<size_t>(result.savings().response_bytes_saved()),
              strlen(kUnminified) - strlen(kMinified));
    ASSERT_EQ(result.resource_urls_size(), 1);
    ASSERT_EQ(result.resource_urls(0), "http://www.example.com/foo.css");

    ASSERT_EQ(score, minify.ComputeScore(
        *pagespeed_input()->input_information(),
        rule_results));
  }

  void CheckErrorInternal(bool save_optimized_content) {
    MinifyCss minify(save_optimized_content);

    RuleResults rule_results;
    ResultProvider provider(minify, &rule_results, 0);
    pagespeed::RuleInput rule_input(*pagespeed_input());
    ASSERT_FALSE(minify.AppendResults(rule_input, &provider));
    ASSERT_EQ(rule_results.results_size(), 0);
  }
};

TEST_F(MinifyCssTest, Basic) {
  AddTestResource("http://www.example.com/foo.css",
                  "text/css",
                  kUnminified);
  Freeze();

  CheckOneViolation(28);
}

TEST_F(MinifyCssTest, WrongContentTypeDoesNotGetMinified) {
  AddTestResource("http://www.example.com/foo.css",
                  "text/html",
                  kUnminified);
  Freeze();

  CheckNoViolations();
}

TEST_F(MinifyCssTest, AlreadyMinified) {
  AddTestResource("http://www.example.com/foo.css",
                  "text/css",
                  kMinified);
  Freeze();

  CheckNoViolations();
}

}  // namespace
