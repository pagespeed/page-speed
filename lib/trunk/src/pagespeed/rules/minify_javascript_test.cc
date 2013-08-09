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
#include "pagespeed/rules/minify_javascript.h"
#include "pagespeed/testing/pagespeed_test.h"

using pagespeed::rules::MinifyJavaScript;
using pagespeed::PagespeedInput;
using pagespeed::Resource;
using pagespeed::Result;
using pagespeed::Results;
using pagespeed::ResultProvider;
using pagespeed::ResultVector;
using pagespeed::RuleResults;
using pagespeed_testing::PagespeedRuleTest;

namespace {

// Unminified JavaScript.
const char* kUnminified = "function () { foo(); }";

// The same JavaScript, minified using JSMin.
const char* kMinified = "function(){foo();}";

// TODO(aoates): combine this with the tests from MinifyCss and MinifyHtml
// (since there's a lot of common code).
class MinifyJavaScriptTest : public PagespeedRuleTest<MinifyJavaScript> {
 protected:
  void AddTestResource(const char* url,
                       const char* content_type,
                       const char* body) {
    Resource* resource = New200Resource(url);
    if (content_type != NULL) {
      resource->AddResponseHeader("Content-Type", content_type);
    }

    if (body != NULL) {
      resource->SetResponseBody(body);
    }
  }
};

TEST_F(MinifyJavaScriptTest, Basic) {
  AddTestResource("http://www.example.com/foo.js",
                  "application/x-javascript",
                  kUnminified);
  CheckOneUrlViolation("http://www.example.com/foo.js");
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

TEST_F(MinifyJavaScriptTest, Format) {
  AddTestResource("http://www.example.com/foo.js",
                  "application/x-javascript",
                  kUnminified);
  Freeze();
  ASSERT_TRUE(AppendResults());
  ASSERT_EQ(
      "Minify JavaScript"
      "<https://developers.google.com/speed/docs/insights/MinifyResources> "
      "for the following resources to reduce their size by 4B "
      "(19% reduction).\n  Minifying "
      "http://www.example.com/foo.js could save 4B (19% reduction).\n",
      FormatResults());
}

}  // namespace
