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
#include "testing/gtest/include/gtest/gtest.h"

using pagespeed::rules::MinifyCSS;
using pagespeed::PagespeedInput;
using pagespeed::Resource;
using pagespeed::Result;
using pagespeed::Results;
using pagespeed::ResultProvider;

namespace {

// Unminified CSS.
const char* kUnminified = "body { color: red /*the color of red things*/; }";

// The same CSS, minified.
const char* kMinified = "body{color:red;}\n";

class MinifyCssTest : public ::testing::Test {
 protected:

  virtual void SetUp() {
    input_.reset(new PagespeedInput);
  }

  virtual void TearDown() {
    input_.reset();
  }

  void AddTestResource(const char* url,
                       const char* content_type,
                       const char* body) {
    Resource* resource = new Resource;
    resource->SetRequestUrl(url);
    resource->SetRequestMethod("GET");
    resource->SetRequestProtocol("HTTP");
    resource->SetResponseStatusCode(200);
    resource->SetResponseProtocol("HTTP/1.1");

    if (content_type != NULL) {
      resource->AddResponseHeader("Content-Type", content_type);
    }

    if (body != NULL) {
      resource->SetResponseBody(body);
    }
    input_->AddResource(resource);
  }

  void CheckNoViolations() {
    CheckNoViolationsInternal(false);
    CheckNoViolationsInternal(true);
  }

  void CheckOneViolation() {
    CheckOneViolationInternal(false);
    CheckOneViolationInternal(true);
  }

  void CheckError() {
    CheckErrorInternal(false);
    CheckErrorInternal(true);
  }

 private:
  void CheckNoViolationsInternal(bool save_optimized_content) {
    MinifyCSS minify(save_optimized_content);

    Results results;
    ResultProvider provider(minify, &results);
    ASSERT_TRUE(minify.AppendResults(*input_, &provider));
    ASSERT_EQ(results.results_size(), 0);
  }

  void CheckOneViolationInternal(bool save_optimized_content) {
    MinifyCSS minify(save_optimized_content);

    Results results;
    ResultProvider provider(minify, &results);
    ASSERT_TRUE(minify.AppendResults(*input_, &provider));
    ASSERT_EQ(results.results_size(), 1);

    const Result& result = results.results(0);

    if (save_optimized_content) {
      ASSERT_TRUE(result.has_optimized_content());
      EXPECT_EQ(kMinified, result.optimized_content());
    } else {
      ASSERT_FALSE(result.has_optimized_content());
    }

    ASSERT_EQ(result.savings().response_bytes_saved(),
              strlen(kUnminified) - strlen(kMinified));
    ASSERT_EQ(result.resource_urls_size(), 1);
    ASSERT_EQ(result.resource_urls(0), "http://www.example.com/foo.css");
  }

  void CheckErrorInternal(bool save_optimized_content) {
    MinifyCSS minify(save_optimized_content);

    Results results;
    ResultProvider provider(minify, &results);
    ASSERT_FALSE(minify.AppendResults(*input_, &provider));
    ASSERT_EQ(results.results_size(), 0);
  }

  scoped_ptr<PagespeedInput> input_;
};

TEST_F(MinifyCssTest, Basic) {
  AddTestResource("http://www.example.com/foo.css",
                  "text/css",
                  kUnminified);

  CheckOneViolation();
}

TEST_F(MinifyCssTest, WrongContentTypeDoesNotGetMinified) {
  AddTestResource("http://www.example.com/foo.css",
                  "text/html",
                  kUnminified);

  CheckNoViolations();
}

TEST_F(MinifyCssTest, AlreadyMinified) {
  AddTestResource("http://www.example.com/foo.css",
                  "text/css",
                  kMinified);

  CheckNoViolations();
}

}  // namespace
