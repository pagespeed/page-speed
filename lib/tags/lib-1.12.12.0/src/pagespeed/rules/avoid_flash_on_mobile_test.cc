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

#include "base/scoped_ptr.h"
#include "base/stl_util-inl.h"  // for STLDeleteContainerPointers
#include "pagespeed/core/dom.h"
#include "pagespeed/core/pagespeed_input.h"
#include "pagespeed/core/resource.h"
#include "pagespeed/core/result_provider.h"
#include "pagespeed/proto/pagespeed_output.pb.h"
#include "pagespeed/rules/avoid_flash_on_mobile.h"
#include "pagespeed/testing/pagespeed_test.h"

namespace {

using pagespeed::rules::AvoidFlashOnMobile;
using pagespeed::Resource;
using pagespeed::Result;
using pagespeed::Results;
using pagespeed::ResultProvider;
using pagespeed_testing::FakeDomDocument;
using pagespeed_testing::FakeDomElement;
using pagespeed_testing::FakeImageAttributesFactory;
using pagespeed_testing::PagespeedRuleTest;

class AvoidFlashOnMobileTest
    : public PagespeedRuleTest<AvoidFlashOnMobile> {
 protected:
  static const char* kRootUrl;

  void AddTestResource(const char* url, const char* content_type) {
    Resource* resource = new Resource;
    resource->SetRequestUrl(url);
    resource->SetRequestMethod("GET");
    resource->SetResponseStatusCode(200);
    resource->AddResponseHeader("Content-Type", content_type);
    AddResource(resource);
  }

  void AddFlashResource(const char* url) {
    AddTestResource(url, "application/x-shockwave-flash");
  }

  virtual void DoSetUp() {
    NewPrimaryResource(kRootUrl);
    CreateHtmlHeadBodyElements();
  }

  void CheckFormattedOutput(const std::string& expected_output) {
    Freeze();
    ASSERT_TRUE(AppendResults());
    EXPECT_EQ(expected_output, FormatResults());
  }
};

const char* AvoidFlashOnMobileTest::kRootUrl = "http://example.com/";

TEST_F(AvoidFlashOnMobileTest, Empty) {
  CheckNoViolations();
}

TEST_F(AvoidFlashOnMobileTest, Flash) {
  static const char* swf = "http://example.com/banner.swf";
  AddFlashResource(swf);
  CheckOneUrlViolation(swf);
}

TEST_F(AvoidFlashOnMobileTest, TwoFlash) {
  static const char* swf1 = "http://example.com/banner?user=test";
  static const char* swf2 = "http://example.com/resources/movie.swf";
  AddFlashResource(swf1);
  AddFlashResource(swf2);
  CheckTwoUrlViolations(swf1, swf2);
}

TEST_F(AvoidFlashOnMobileTest, Silverlight) {
  AddTestResource("http://example.com/SilverlightApplication1.xap",
                  "application/x-silverlight-2");
  // Only testing for Adobe Flash
  CheckNoViolations();
}

TEST_F(AvoidFlashOnMobileTest, FormatNoOutputTest) {
  CheckFormattedOutput("");
}

TEST_F(AvoidFlashOnMobileTest, FormatTest) {
  std::string expected =
      "The following Flash objects are included on the page. "
      "Adobe Flash Player is not supported on Apple iOS or Android "
      "versions greater than 4.0.x+. Consider removing Flash objects and "
      "finding suitable replacements.\n"
      "  http://example.com/banner.swf\n";
  AddFlashResource("http://example.com/banner.swf");
  CheckFormattedOutput(expected);
}

}  // namespace
