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
#include "pagespeed/rules/minify_html.h"
#include "pagespeed/testing/pagespeed_test.h"

using pagespeed::rules::MinifyHTML;
using pagespeed::PagespeedInput;
using pagespeed::Resource;
using pagespeed::Result;
using pagespeed::Results;
using pagespeed::ResultProvider;
using pagespeed::ResultVector;
using pagespeed::RuleResults;
using pagespeed_testing::PagespeedRuleTest;

namespace {

// Unminified HTML.
const char* kUnminified =
    "<html>\n"
    "  <head>\n"
    "    <title>Foo</title>\n"
    "    <script>\n"
    "      var foo = 42;\n"
    "    </script>\n"
    "  </head>\n"
    "  <body>\n"
    "    Foo!\n"
    "  </body>\n"
    "</html>\n";

// The same HTML, minified.
const char* kMinified =
    "<html>\n"
    "<head>\n"
    "<title>Foo</title>\n"
    "<script>var foo=42;</script>\n"
    "</head>\n"
    "<body>\n"
    "Foo!\n"
    "</body>\n"
    "</html>\n";

class MinifyHtmlTest : public PagespeedRuleTest<MinifyHTML> {
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

TEST_F(MinifyHtmlTest, Basic) {
  AddTestResource("http://www.example.com/foo.html",
                  "text/html",
                  kUnminified);
  CheckOneUrlViolation("http://www.example.com/foo.html");
}

TEST_F(MinifyHtmlTest, WrongContentTypeDoesNotGetMinified) {
  AddTestResource("http://www.example.com/foo.html",
                  "text/css",
                  kUnminified);
  CheckNoViolations();
}

TEST_F(MinifyHtmlTest, AlreadyMinified) {
  AddTestResource("http://www.example.com/foo.html",
                  "text/html",
                  kMinified);
  CheckNoViolations();
}

TEST_F(MinifyHtmlTest, Format) {
  AddTestResource("http://www.example.com/foo.html",
                  "text/html",
                  kUnminified);
  Freeze();
  ASSERT_TRUE(AppendResults());
  ASSERT_EQ(
      "Compacting HTML code, including any inline JavaScript and "
      "CSS contained in it, can save many bytes of data and speed up "
      "download and parse times.\n"
      "Minify HTML<"
      "https://developers.google.com/speed/docs/insights/MinifyResources> "
      "for the following resources to reduce their size by 34B "
      "(26% reduction).\n  Minifying "
      "http://www.example.com/foo.html could save 34B (26% reduction).\n",
      FormatResults());
}

TEST_F(MinifyHtmlTest, FormatNoResults) {
  Freeze();
  ASSERT_TRUE(AppendResults());
  ASSERT_EQ(
      "Your HTML is minified. Learn more about minifying HTML"
      "<https://developers.google.com/speed/docs/insights/MinifyResources>.\n",
      FormatResults());
}

}  // namespace
