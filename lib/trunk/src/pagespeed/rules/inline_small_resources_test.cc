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
#include <vector>

#include "pagespeed/rules/inline_small_resources.h"
#include "pagespeed/testing/pagespeed_test.h"

using pagespeed::InlineSmallResourcesDetails;
using pagespeed::Resource;

using pagespeed::rules::InlineSmallCss;
using pagespeed::rules::InlineSmallJavaScript;
using pagespeed::rules::InlineSmallResources;

namespace {

static const size_t kInlineThresholdBytes = 768;

// Would prefer to use const char* here but these need to be inlined
// in other constant strings, which doesn't seem possible using const
// char*.
#define SAME_DOMAIN_URL_1 "http://www.example.com/a.css"
#define SAME_DOMAIN_URL_2 "http://www.example.com/b.css"
#define DIFF_DOMAIN_URL_1 "http://www.foo.com/b.css"
#define IFRAME_URL        "http://www.example.com/iframe.html"

static const char* kRootUrl = "http://www.example.com/index.html";

static const char* kHtmlNoCss = "<html><body></body></html>";
static const char* kHtmlOneCssSameDomain =
    "<html><head><link rel='stylesheet' href='"
    SAME_DOMAIN_URL_1
    "'></head><body></body></html>";

static const char* kHtmlOneCssDiffDomain =
    "<html><head><link rel='stylesheet' href='"
    DIFF_DOMAIN_URL_1
    "'></head><body></body></html>";

static const char* kHtmlTwoCssSameDomain =
    "<html><head>"
    "<link rel='stylesheet' href='"
    SAME_DOMAIN_URL_1
    "'>"
    "<link rel='stylesheet' href='"
    SAME_DOMAIN_URL_2
    "'>"
    "</head><body></body></html>";

static const char* kHtmlTwoCssOneSameDomain =
    "<html><head>"
    "<link rel='stylesheet' href='"
    DIFF_DOMAIN_URL_1
    "'>"
    "<link rel='stylesheet' href='"
    SAME_DOMAIN_URL_1
    "'>"
    "</head><body></body></html>";

static const char* kHtmlOneCssTwoDifferentFrames =
    "<html><head>"
    "<link rel='stylesheet' href='"
    SAME_DOMAIN_URL_1
    "'>"
    "<iframe src='"
    IFRAME_URL
    "'>"
    "</head><body></body></html>";

static const char* kHtmlForIframe =
    "<html><head>"
    "<link rel='stylesheet' href='"
    SAME_DOMAIN_URL_1
    "'>"
    "</head><body></body></html>";

template <class RULE> class InlineSmallResourcesTest :
      public pagespeed_testing::PagespeedRuleTest<RULE> {
 protected:
  virtual void DoSetUp() {
    this->NewPrimaryResource(kRootUrl);
  }

  void CheckNoViolations() {
    CheckExpectedViolations(std::vector<std::string>());
  }

  void CheckOneViolation(const char* url) {
    std::vector<std::string> expected;
    expected.push_back(url);
    CheckExpectedViolations(expected);
  }

  void CheckTwoViolations(const char* url1, const char* url2) {
    std::vector<std::string> expected;
    expected.push_back(url1);
    expected.push_back(url2);
    CheckExpectedViolations(expected);
  }

 private:
  void CheckExpectedViolations(
      const std::vector<std::string>& expected) {
    this->Freeze();
    ASSERT_TRUE(this->AppendResults());
    if (expected.size() == 0) {
      ASSERT_EQ(0, this->num_results());
      return;
    }

    ASSERT_EQ(1, this->num_results());
    ASSERT_EQ(1, this->result(0).resource_urls_size());
    ASSERT_EQ(kRootUrl, this->result(0).resource_urls(0));
    const pagespeed::ResultDetails& details = this->result(0).details();
    ASSERT_TRUE(details.HasExtension(
        InlineSmallResourcesDetails::message_set_extension));
    const InlineSmallResourcesDetails& isr_details = details.GetExtension(
        InlineSmallResourcesDetails::message_set_extension);
    ASSERT_EQ(expected.size(),
              static_cast<size_t>(isr_details.inline_candidates_size()));
    for (size_t idx = 0; idx < expected.size(); ++idx) {
      ASSERT_EQ(expected[idx], isr_details.inline_candidates(idx));
    }
  }
};

// Since the logic in InlineSmallCss and InlineSmallJavaScript is the
// same, we only write tests for one and assume that we are exercising
// the code in both.
class InlineSmallCssTest : public InlineSmallResourcesTest<InlineSmallCss> {};

TEST_F(InlineSmallCssTest, OneHtmlResource) {
  primary_resource()->SetResponseBody(kHtmlNoCss);
  CheckNoViolations();
}

TEST_F(InlineSmallCssTest, LargeExternalFileSameDomain) {
  std::string large_css;
  large_css.append(kInlineThresholdBytes, 'x');
  NewCssResource(SAME_DOMAIN_URL_1, NULL, NULL)->SetResponseBody(large_css);
  primary_resource()->SetResponseBody(kHtmlOneCssSameDomain);
  CheckNoViolations();
}

TEST_F(InlineSmallCssTest, LargeMinifiableFileSameDomain) {
  // Make sure that we are using the post-minified size of the
  // resource when deciding whether or not to inline. In this case,
  // create a resource full of kInlineThresholdBytes spaces, which
  // will minify to zero bytes. It should thus be a candidate for
  // inlining.
  std::string large_css;
  large_css.append(kInlineThresholdBytes, ' ');
  NewCssResource(SAME_DOMAIN_URL_1, NULL, NULL)->SetResponseBody(large_css);
  primary_resource()->SetResponseBody(kHtmlOneCssSameDomain);
  CheckOneViolation(SAME_DOMAIN_URL_1);
}

TEST_F(InlineSmallCssTest, SmallExternalFileSameDomain) {
  NewCssResource(SAME_DOMAIN_URL_1, NULL, NULL)->SetResponseBody("");
  primary_resource()->SetResponseBody(kHtmlOneCssSameDomain);
  CheckOneViolation(SAME_DOMAIN_URL_1);
}

TEST_F(InlineSmallCssTest, SmallExternalFileDiffDomain) {
  NewCssResource(DIFF_DOMAIN_URL_1, NULL, NULL)->SetResponseBody("");
  primary_resource()->SetResponseBody(kHtmlOneCssDiffDomain);
  CheckNoViolations();
}

TEST_F(InlineSmallCssTest, TwoSmallExternalFilesSameDomain) {
  NewCssResource(SAME_DOMAIN_URL_1, NULL, NULL)->SetResponseBody("");
  NewCssResource(SAME_DOMAIN_URL_2, NULL, NULL)->SetResponseBody("");
  primary_resource()->SetResponseBody(kHtmlTwoCssSameDomain);
  CheckTwoViolations(SAME_DOMAIN_URL_1, SAME_DOMAIN_URL_2);
}

TEST_F(InlineSmallCssTest, TwoSmallExternalFilesOneSameDomain) {
  NewCssResource(SAME_DOMAIN_URL_1, NULL, NULL)->SetResponseBody("");
  NewCssResource(DIFF_DOMAIN_URL_1, NULL, NULL)->SetResponseBody("");
  primary_resource()->SetResponseBody(kHtmlTwoCssOneSameDomain);
  CheckOneViolation(SAME_DOMAIN_URL_1);
}

TEST_F(InlineSmallCssTest, OneSmallExternalFileTwoDifferentFrames) {
  NewCssResource(SAME_DOMAIN_URL_1, NULL, NULL)->SetResponseBody("");
  primary_resource()->SetResponseBody(kHtmlOneCssTwoDifferentFrames);
  NewDocumentResource(IFRAME_URL)->SetResponseBody(kHtmlForIframe);
  CheckNoViolations();
}

}  // namespace
