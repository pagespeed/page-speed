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
using pagespeed_testing::PagespeedRuleTest;

namespace {

// Unminified CSS.
const char* kUnminified = "body { color: red /*red*/; }";

// The same CSS, minified.
const char* kMinified = "body{color:red;}";

class MinifyCssTest : public PagespeedRuleTest<MinifyCss> {
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

TEST_F(MinifyCssTest, Basic) {
  AddTestResource("http://www.example.com/foo.css",
                  "text/css",
                  kUnminified);
  CheckOneUrlViolation("http://www.example.com/foo.css");
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

TEST_F(MinifyCssTest, Format) {
  AddTestResource("http://www.example.com/foo.css",
                  "text/css",
                  kUnminified);
  Freeze();
  ASSERT_TRUE(AppendResults());
  ASSERT_EQ(
      "Compacting CSS code can save many bytes of data "
      "and speed up download and parse times.\n"
      "Minify CSS"
      "<https://developers.google.com/speed/docs/insights/MinifyResources> "
      "for the following resources to reduce their size by 12B "
      "(43% reduction).\n  Minifying "
      "http://www.example.com/foo.css could save 12B (43% reduction).\n",
      FormatResults());
}

TEST_F(MinifyCssTest, FormatNoResults) {
  Freeze();
  ASSERT_TRUE(AppendResults());
  ASSERT_EQ(
      "Your CSS is minified. Learn more about minifying CSS"
      "<https://developers.google.com/speed/docs/insights/MinifyResources>.\n",
      FormatResults());
}

}  // namespace
