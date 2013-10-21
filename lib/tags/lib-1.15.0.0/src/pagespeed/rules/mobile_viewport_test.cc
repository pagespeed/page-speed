// Copyright 2012 Google Inc. All Rights Reserved.
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
#include <vector>

#include "pagespeed/core/result_provider.h"
#include "pagespeed/proto/pagespeed_output.pb.h"
#include "pagespeed/rules/mobile_viewport.h"
#include "pagespeed/testing/pagespeed_test.h"

using pagespeed::Result;
using pagespeed::rules::MobileViewport;
using pagespeed::ResultProvider;
using pagespeed::RuleResults;

namespace {

const char* kHtmlWithoutViewport =
    "<html><head>"
    "</head><body>Hello, world.</body></html>";

const char* kHtmlWithMetaNameViewport =
    "<html><head>"
    "<meta name=\"viewport\" content=\"width=device-width\">\n"
    "</head><body>Hello, world.</body></html>";

const char* kHtmlWithOtherMetaTags =
    "<html><head>"
    "<meta http-equiv=\"Content-Type\" content=\"text/html\" >\n"
    "<meta name=\"keywords\" content=\"viewport,tests\" >\n"
    "<META NAME=\"ROBOTS\" CONTENT=\"NOINDEX, NOFOLLOW\">\n"
    "</head><body>Hello, world.</body></html>";

const char* kHtmlWithMetaNameViewportAllCaps =
    "<HTML><HEAD>"
    "<META NAME=\"VIEWPORT\" CONTENT=\"WIDTH=DEVICE-WIDTH\">\n"
    "</HEAD><BODY>HELLO, WORLD!</BODY></HTML>";

const char* kRootUrl = "http://www.example.com/";

class MobileViewportTest : public
    ::pagespeed_testing::PagespeedRuleTest<MobileViewport> {
 protected:
  virtual void DoSetUp() {
    NewPrimaryResource(kRootUrl);
  }

  void CheckFormattedOutput(const std::string& expected_output) {
   Freeze();
   ASSERT_TRUE(AppendResults());
   EXPECT_EQ(expected_output, FormatResults());
  }
};

TEST_F(MobileViewportTest, NoViewport) {
  primary_resource()->SetResponseBody(kHtmlWithoutViewport);
  CheckOneUrlViolation(kRootUrl);
}

TEST_F(MobileViewportTest, MetaViewport) {
  primary_resource()->SetResponseBody(kHtmlWithMetaNameViewport);
  CheckNoViolations();
}

TEST_F(MobileViewportTest, NoViewportOtherMetaTags) {
  primary_resource()->SetResponseBody(kHtmlWithOtherMetaTags);
  CheckOneUrlViolation(kRootUrl);
}

TEST_F(MobileViewportTest, MetaViewportAllCaps) {
  primary_resource()->SetResponseBody(kHtmlWithMetaNameViewportAllCaps);
  CheckNoViolations();
}

TEST_F(MobileViewportTest, FormatTest) {
  std::string expected =
      "The following pages do not specify a viewport. Consider adding a meta "
      "tag specifying a viewport so mobile browsers can render the document at "
      "a usable size.\n"
      "  http://www.example.com/\n";
  primary_resource()->SetResponseBody(kHtmlWithoutViewport);
  CheckFormattedOutput(expected);
}

TEST_F(MobileViewportTest, FormatNoOutputTest) {
  primary_resource()->SetResponseBody(kHtmlWithMetaNameViewport);
  CheckFormattedOutput("");
}

}  // namespace
