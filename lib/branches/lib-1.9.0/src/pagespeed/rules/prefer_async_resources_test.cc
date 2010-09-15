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
#include <vector>

#include "pagespeed/rules/prefer_async_resources.h"
#include "pagespeed/testing/pagespeed_test.h"

using pagespeed::DomDocument;
using pagespeed::PreferAsyncResourcesDetails;
using pagespeed::Resource;

using pagespeed::rules::PreferAsyncResources;
using pagespeed_testing::FakeDomDocument;
using pagespeed_testing::FakeDomElement;

namespace {

class PreferAsyncResourcesTest
    : public ::pagespeed_testing::PagespeedRuleTest<PreferAsyncResources> {
 protected:
  static const char* kRootUrl;
  static const char* kIframeUrl;

  virtual void DoSetUp() {
    NewPrimaryResource(kRootUrl);
    CreateHtmlHeadBodyElements();
  }

  FakeDomElement* CreatePngElement(FakeDomElement* parent) {
    FakeDomElement* element;
    NewPngResource("http://test.com/test.png", parent, &element);
    return element;
  }

  FakeDomElement* CreateCssElement(FakeDomElement* parent) {
    FakeDomElement* element;
    NewCssResource("http://test.com/test.css", parent, &element);
    return element;
  }

  FakeDomElement* CreateScriptElement(
      const std::string& url, FakeDomElement* parent) {
    FakeDomElement* element;
    NewScriptResource(url, parent, &element);
    return element;
  }

  FakeDomElement* CreateIframeElement(FakeDomElement* parent) {
    FakeDomElement* iframe = FakeDomElement::NewIframe(body());
    FakeDomDocument* iframe_doc;
    NewDocumentResource(kIframeUrl, iframe, &iframe_doc);
    return FakeDomElement::NewRoot(iframe_doc, "html");
  }

  void CheckNoViolations() {
    CheckExpectedViolations(std::vector<Violation>());
  }

  void CheckOneViolation(
      const std::string& document_url, const std::string& resource_url) {
    std::vector<Violation> expected;
    expected.push_back(Violation(document_url, resource_url));
    CheckExpectedViolations(expected);
  }

  void CheckTwoViolations(
      const std::string& document_url1, const std::string& resource_url1,
      const std::string& document_url2, const std::string& resource_url2) {
    std::vector<Violation> expected;
    expected.push_back(Violation(document_url1, resource_url1));
    expected.push_back(Violation(document_url2, resource_url2));
    CheckExpectedViolations(expected);
  }

  void CheckFormattedOutput(const std::string& expected_output) {
    Freeze();

    ASSERT_TRUE(AppendResults());
    EXPECT_EQ(expected_output, FormatResults());
  }

 private:
  struct Violation {
    Violation(
        const std::string& document_url, const std::string& resource_url)
        : document_url_(document_url), resource_url_(resource_url) {}
    std::string document_url_;
    std::string resource_url_;
  };

  void CheckExpectedViolations(
      const std::vector<Violation>& expected) {
    Freeze();

    ASSERT_TRUE(AppendResults());
    ASSERT_EQ(expected.size(), static_cast<size_t>(num_results()));

    for (size_t idx = 0; idx < expected.size(); ++idx) {
      ASSERT_EQ(result(idx).resource_urls_size(), 1);
      EXPECT_EQ(expected[idx].document_url_, result(idx).resource_urls(0));
      const pagespeed::ResultDetails& details = result(idx).details();
      ASSERT_TRUE(details.HasExtension(
          PreferAsyncResourcesDetails::message_set_extension));
      const PreferAsyncResourcesDetails& async_details = details.GetExtension(
          PreferAsyncResourcesDetails::message_set_extension);
      EXPECT_EQ(expected[idx].resource_url_, async_details.resource_url());
    }
  }
};

// A special test class that overrides the root URL to the analytics
// root. We need to do this in order to verify that our URL resolving
// code works correctly.
class PreferAsyncResourcesRelativeTest : public PreferAsyncResourcesTest {
 protected:
  static const char* kRelativeRootUrl;
  virtual void DoSetUp() {
    NewPrimaryResource(kRelativeRootUrl);
    CreateHtmlHeadBodyElements();
  }
};

const char* PreferAsyncResourcesTest::kRootUrl = "http://test.com/";
const char* PreferAsyncResourcesTest::kIframeUrl = "http://test.com/iframe.htm";
const char* PreferAsyncResourcesRelativeTest::kRelativeRootUrl =
    "http://www.google-analytics.com/index.html";

TEST_F(PreferAsyncResourcesTest, EmptyDom) {
  CheckNoViolations();
}

TEST_F(PreferAsyncResourcesTest, AsyncGoogleAnalyticsIsOkay) {
  FakeDomElement* ga_script = CreateScriptElement(
      "http://www.google-analytics.com/ga.js", body());
  ga_script->AddAttribute("async", "true");
  CreatePngElement(body());
  CreateCssElement(body());
  CreateScriptElement("http://test.com/test.js", body());
  CheckNoViolations();
}

TEST_F(PreferAsyncResourcesTest, SyncGoogleAnalyticsLastIsBad) {
  CreatePngElement(body());
  CreateCssElement(body());
  CreateScriptElement("http://test.com/test.js", body());
  CreateScriptElement("http://www.google-analytics.com/ga.js", body());
  CheckOneViolation(kRootUrl, "http://www.google-analytics.com/ga.js");
}

TEST_F(PreferAsyncResourcesTest, SyncGoogleAnalyticsAboveCssIsBad) {
  CreateScriptElement("http://www.google-analytics.com/ga.js", body());
  CreateCssElement(body());
  CheckOneViolation(kRootUrl, "http://www.google-analytics.com/ga.js");
}

TEST_F(PreferAsyncResourcesTest, SyncGoogleAnalyticsExtendedUrlIsOk) {
  CreateScriptElement("http://www.google-analytics.com/ga.jsfoo", body());
  CreateCssElement(body());
  CheckNoViolations();
}

TEST_F(PreferAsyncResourcesTest, LazyLoadedSyncGoogleAnalyticsAboveCssIsOk) {
  NewScriptResource(
      "http://www.google-analytics.com/ga.js", body())->SetLazyLoaded();
  CreateCssElement(body());
  CheckNoViolations();
}

TEST_F(PreferAsyncResourcesTest, SyncGoogleAnalyticsAboveCssWithHttpsIsBad) {
  CreateScriptElement("https://ssl.google-analytics.com/ga.js", body());
  CreateCssElement(body());
  CheckOneViolation(kRootUrl, "https://ssl.google-analytics.com/ga.js");
}

TEST_F(PreferAsyncResourcesTest, SyncGoogleAnalyticsAboveImageIsBad) {
  CreateScriptElement("http://www.google-analytics.com/ga.js", body());
  CreatePngElement(body());
  CheckOneViolation(kRootUrl, "http://www.google-analytics.com/ga.js");
}

TEST_F(PreferAsyncResourcesTest, SyncGoogleAnalyticsAboveScriptIsBad) {
  CreateScriptElement("http://www.google-analytics.com/ga.js", body());
  CreateScriptElement("http://test.com/test.js", body());
  CheckOneViolation(kRootUrl, "http://www.google-analytics.com/ga.js");
}

TEST_F(PreferAsyncResourcesTest, SyncGoogleAnalyticsAboveIframeIsBad) {
  CreateScriptElement("http://www.google-analytics.com/ga.js", body());
  CreateIframeElement(body());
  CheckOneViolation(kRootUrl, "http://www.google-analytics.com/ga.js");
}

TEST_F(PreferAsyncResourcesTest, UrchinGoogleAnalyticsAboveOtherContentIsBad) {
  CreateScriptElement("http://www.google-analytics.com/urchin.js", body());
  CreateCssElement(body());
  CheckOneViolation(kRootUrl, "http://www.google-analytics.com/urchin.js");
}

TEST_F(PreferAsyncResourcesTest,
       UrchinAndSyncGoogleAnalyticsAboveOtherConentIsBad) {
  CreateScriptElement("http://www.google-analytics.com/urchin.js", body());
  CreateCssElement(body());
  CheckOneViolation(kRootUrl, "http://www.google-analytics.com/urchin.js");
}

TEST_F(PreferAsyncResourcesTest, GoogleAnalyticsMixedResults) {
  CreateScriptElement("http://www.google-analytics.com/ga.js", body());
  CreateScriptElement("http://test.com/test.js", body());
  CreateScriptElement("http://www.google-analytics.com/urchin.js", body());
  CheckTwoViolations(kRootUrl, "http://www.google-analytics.com/ga.js",
                     kRootUrl, "http://www.google-analytics.com/urchin.js");
}

TEST_F(PreferAsyncResourcesTest, GoogleAnalyticsTwoViolations) {
  CreateScriptElement("http://www.google-analytics.com/ga.js", body());
  CreateScriptElement("http://www.google-analytics.com/urchin.js", body());
  CreateScriptElement("http://test.com/test.js", body());
  CheckTwoViolations(kRootUrl, "http://www.google-analytics.com/ga.js",
                     kRootUrl, "http://www.google-analytics.com/urchin.js");
}

TEST_F(PreferAsyncResourcesTest, FormatTest) {
  std::string expected =
      "The following resources are loaded synchronously. Load them "
      "asynchronously to reduce blocking of page rendering.\n"
      "http://test.com/ loads http://www.google-analytics.com/ga.js "
      "synchronously.\n";
  CreateScriptElement("http://www.google-analytics.com/ga.js", body());
  CreateCssElement(body());
  CheckFormattedOutput(expected);
}

TEST_F(PreferAsyncResourcesTest, FormatNoOutputTest) {
  CheckFormattedOutput("");
}

TEST_F(PreferAsyncResourcesTest, SyncGoogleAnalyticsInIframeIsBad) {
  FakeDomElement* iframe_root = CreateIframeElement(body());
  CreateScriptElement("http://www.google-analytics.com/ga.js", iframe_root);
  CheckOneViolation(kIframeUrl, "http://www.google-analytics.com/ga.js");
}

TEST_F(PreferAsyncResourcesTest, SyncGoogleAnalyticsInIframeAboveCssIsBad) {
  FakeDomElement* iframe_root = CreateIframeElement(body());
  CreateScriptElement("http://www.google-analytics.com/ga.js", iframe_root);
  CreateCssElement(iframe_root);
  CheckOneViolation(kIframeUrl, "http://www.google-analytics.com/ga.js");
}

TEST_F(PreferAsyncResourcesTest, SyncScriptInBodyAndIframeIsDoublyBad) {
  CreateScriptElement("http://www.google-analytics.com/urchin.js", body());
  FakeDomElement* iframe_root = CreateIframeElement(body());
  CreateScriptElement("http://www.google-analytics.com/ga.js", iframe_root);
  CreateCssElement(iframe_root);
  CheckTwoViolations(kIframeUrl, "http://www.google-analytics.com/ga.js",
                     kRootUrl, "http://www.google-analytics.com/urchin.js");
}

// Make sure the DOM traversal properly resolves relative URLs.
TEST_F(PreferAsyncResourcesRelativeTest, SyncGoogleAnalyticsRelativeUrl) {
  FakeDomElement* element;
  NewScriptResource("http://www.google-analytics.com/ga.js",
                    body(),
                    &element);
  element->AddAttribute("src", "ga.js");
  CreateCssElement(body());
  CheckOneViolation(kRelativeRootUrl,
                    "http://www.google-analytics.com/ga.js");
}

}  // namespace
