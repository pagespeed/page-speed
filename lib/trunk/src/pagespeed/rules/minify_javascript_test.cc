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
#include "pagespeed/proto/pagespeed_output.pb.h"
#include "pagespeed/rules/minify_javascript.h"
#include "testing/gtest/include/gtest/gtest.h"

using pagespeed::rules::MinifyJavaScript;
using pagespeed::PagespeedInput;
using pagespeed::Resource;
using pagespeed::Result;
using pagespeed::Results;

namespace {

// Unminified JavaScript.
const char* kUnminified = "function () { foo(); }";

// The same JavaScript, minified using JSMin. Notice that JSMin
// prepends a newline to its output.
const char* kMinified = "\nfunction(){foo();}";

class MinifyJavaScriptTest : public ::testing::Test {
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
    MinifyJavaScript minify;

    Results results;
    ASSERT_TRUE(minify.AppendResults(*input_, &results));
    ASSERT_EQ(results.results_size(), 0);
  }

  void CheckOneViolation() {
    MinifyJavaScript minify;

    Results results;
    ASSERT_TRUE(minify.AppendResults(*input_, &results));
    ASSERT_EQ(results.results_size(), 1);

    const Result& result = results.results(0);
    ASSERT_EQ(result.savings().response_bytes_saved(),
              strlen(kUnminified) - strlen(kMinified));
    ASSERT_EQ(result.resource_urls_size(), 1);
    ASSERT_EQ(result.resource_urls(0), "http://www.example.com/foo.js");
  }

  void CheckError() {
    MinifyJavaScript minify;

    Results results;
    ASSERT_FALSE(minify.AppendResults(*input_, &results));
    ASSERT_EQ(results.results_size(), 0);
  }

 private:
  scoped_ptr<PagespeedInput> input_;
};

TEST_F(MinifyJavaScriptTest, Basic) {
  AddTestResource("http://www.example.com/foo.js",
                  "application/x-javascript",
                  kUnminified);

  CheckOneViolation();
}

TEST_F(MinifyJavaScriptTest, WrongContentTypeDoesNotGetMinified) {
  AddTestResource("http://www.example.com/foo.js",
                  "text/html",
                  kUnminified);

  CheckNoViolations();
}

TEST_F(MinifyJavaScriptTest, AlreadyMinified) {
  AddTestResource("http://www.example.com/foo.js",
                  "application/x-javascript",
                  kMinified);

  CheckNoViolations();
}

TEST_F(MinifyJavaScriptTest, Error) {
  AddTestResource("http://www.example.com/foo.js",
                  "application/x-javascript",
                  "/* not valid javascript");

  CheckError();
}

}  // namespace
