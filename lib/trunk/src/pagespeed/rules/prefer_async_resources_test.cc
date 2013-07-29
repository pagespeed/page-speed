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

  static const char* kGaScriptUrl;
  static const char* kUrchinScriptUrl;
  static const char* kFacebookScriptEnUsUrl;
  static const char* kFacebookScriptEnGbUrl;
  static const char* kFacebookScriptAcceptedUrl;
  static const char* kFacebookScriptRejectedUrl;
  static const char* kPlusOneScriptUrl;
  static const char* kTwitterScriptUrl;
  static const char* kQuantCastScriptUrl;
  static const char* kComScoreScriptUrl;
  static const char* kComScoreSecureScriptUrl;
  static const char* kGptScriptUrl;
  static const char* kShareThisScriptUrl;
  static const char* kPinterestScriptUrl;
  static const char* kDisqusScriptUrl;
  static const char* kDisqusEmbedScriptUrl;
  static const char* kChartBeatScriptUrl;
  static const char* kNewRelicScriptUrl;
  static const char* kClickyScriptUrl;
  static const char* kBuySellAdsScriptUrl;
  static const char* kStumbleUponScriptUrl;
  static const char* kYandexScriptUrl;
  static const char* kTyntScriptUrl;
  static const char* kAdSenseShowAdsUrl1;
  static const char* kAdSenseShowAdsUrl2;

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

const char* PreferAsyncResourcesTest::kRootUrl = "http://test.com/#foo";
const char* PreferAsyncResourcesTest::kIframeUrl = "http://test.com/iframe.htm";
const char* PreferAsyncResourcesRelativeTest::kRelativeRootUrl =
    "http://www.google-analytics.com/index.html";

const char* PreferAsyncResourcesTest::kGaScriptUrl =
    "http://www.google-analytics.com/ga.js";
const char* PreferAsyncResourcesTest::kUrchinScriptUrl =
    "http://www.google-analytics.com/urchin.js";
const char* PreferAsyncResourcesTest::kFacebookScriptEnUsUrl =
    "http://connect.facebook.net/en_US/all.js";
const char* PreferAsyncResourcesTest::kFacebookScriptEnGbUrl =
    "http://connect.facebook.net/en_GB/all.js";
const char* PreferAsyncResourcesTest::kPlusOneScriptUrl =
    "http://apis.google.com/js/plusone.js";
const char* PreferAsyncResourcesTest::kTwitterScriptUrl =
    "http://platform.twitter.com/widgets.js";
const char* PreferAsyncResourcesTest::kQuantCastScriptUrl =
    "http://quantserve.com/quant.js";
const char* PreferAsyncResourcesTest::kComScoreScriptUrl =
    "http://b.scorecardresearch.com/beacon.js";
const char* PreferAsyncResourcesTest::kComScoreSecureScriptUrl =
    "https://sb.scorecardresearch.com/beacon.js";
const char* PreferAsyncResourcesTest::kGptScriptUrl =
    "http://www.googletagservices.com/tag/js/gpt.js";
const char* PreferAsyncResourcesTest::kShareThisScriptUrl =
    "http://w.sharethis.com/button/buttons.js";
const char* PreferAsyncResourcesTest::kPinterestScriptUrl =
    "http://assets.pinterest.com/js/pinit.js";
const char* PreferAsyncResourcesTest::kDisqusScriptUrl =
    "http://example.disqus.com/count.js";
const char* PreferAsyncResourcesTest::kDisqusEmbedScriptUrl =
    "http://example.disqus.com/embed.js";
const char* PreferAsyncResourcesTest::kChartBeatScriptUrl =
    "http://static.chartbeat.com/js/chartbeat.js";
const char* PreferAsyncResourcesTest::kNewRelicScriptUrl =
    "http://d1ros97qkrwjf5.cloudfront.net/42/eum/rum.js";
const char* PreferAsyncResourcesTest::kClickyScriptUrl =
    "http://static.getclicky.com/js";
const char* PreferAsyncResourcesTest::kBuySellAdsScriptUrl =
    "http://s3.buysellads.com/ac/bsa.js";
const char* PreferAsyncResourcesTest::kStumbleUponScriptUrl =
    "http://platform.stumbleupon.com/1/widgets.js";
const char* PreferAsyncResourcesTest::kYandexScriptUrl =
    "http://mc.yandex.ru/metrika/watch.js";
const char* PreferAsyncResourcesTest::kTyntScriptUrl =
    "http://cdn.tynt.com/tc.js";
const char* PreferAsyncResourcesTest::kAdSenseShowAdsUrl1 =
    "http://pagead2.googlesyndication.com/pagead/show_ads.js";
const char* PreferAsyncResourcesTest::kAdSenseShowAdsUrl2 =
    "https://pagead2.googlesyndication.com/pagead/show_ads.js";

// This URL isn't valid for getting the FB js, however it should match
// our matcher.
const char* PreferAsyncResourcesTest::kFacebookScriptAcceptedUrl =
    "http://connect.facebook.net//all.js";

// This URL isn't valid either, and it too should not match our
// matcher.
const char* PreferAsyncResourcesTest::kFacebookScriptRejectedUrl =
    "http://connect.facebook.net/all.js";

TEST_F(PreferAsyncResourcesTest, EmptyDom) {
  CheckNoViolations();
}

TEST_F(PreferAsyncResourcesTest, AsyncGoogleAnalyticsIsOkay) {
  FakeDomElement* ga_script = CreateScriptElement(kGaScriptUrl, body());
  ga_script->AddAttribute("async", "");
  CreatePngElement(body());
  CreateCssElement(body());
  CreateScriptElement("http://test.com/test.js", body());
  CheckNoViolations();
}

TEST_F(PreferAsyncResourcesTest, SyncGoogleAnalyticsLastIsBad) {
  CreatePngElement(body());
  CreateCssElement(body());
  CreateScriptElement("http://test.com/test.js", body());
  CreateScriptElement(kGaScriptUrl, body());
  CheckOneViolation(kRootUrl, kGaScriptUrl);
}

TEST_F(PreferAsyncResourcesTest, SyncGoogleAnalyticsAboveCssIsBad) {
  CreateScriptElement(kGaScriptUrl, body());
  CreateCssElement(body());
  CheckOneViolation(kRootUrl, kGaScriptUrl);
}

TEST_F(PreferAsyncResourcesTest, SyncGoogleAnalyticsExtendedUrlIsOk) {
  CreateScriptElement("http://www.google-analytics.com/ga.jsfoo", body());
  CreateCssElement(body());
  CheckNoViolations();
}

TEST_F(PreferAsyncResourcesTest, PostOnloadSyncGoogleAnalyticsAboveCssIsOk) {
  SetOnloadTimeMillis(10);
  NewScriptResource(kGaScriptUrl,
                    body())->SetRequestStartTimeMillis(11);
  CreateCssElement(body());
  CheckNoViolations();
}

TEST_F(PreferAsyncResourcesTest, SyncGoogleAnalyticsAboveCssWithHttpsIsBad) {
  CreateScriptElement("https://ssl.google-analytics.com/ga.js", body());
  CreateCssElement(body());
  CheckOneViolation(kRootUrl, "https://ssl.google-analytics.com/ga.js");
}

TEST_F(PreferAsyncResourcesTest, SyncGoogleAnalyticsAboveImageIsBad) {
  CreateScriptElement(kGaScriptUrl, body());
  CreatePngElement(body());
  CheckOneViolation(kRootUrl, kGaScriptUrl);
}

TEST_F(PreferAsyncResourcesTest, SyncGoogleAnalyticsAboveScriptIsBad) {
  CreateScriptElement(kGaScriptUrl, body());
  CreateScriptElement("http://test.com/test.js", body());
  CheckOneViolation(kRootUrl, kGaScriptUrl);
}

TEST_F(PreferAsyncResourcesTest, SyncGoogleAnalyticsAboveIframeIsBad) {
  CreateScriptElement(kGaScriptUrl, body());
  CreateIframeElement(body());
  CheckOneViolation(kRootUrl, kGaScriptUrl);
}

TEST_F(PreferAsyncResourcesTest, UrchinGoogleAnalyticsAboveOtherContentIsBad) {
  CreateScriptElement(kUrchinScriptUrl, body());
  CreateCssElement(body());
  CheckOneViolation(kRootUrl, kUrchinScriptUrl);
}

TEST_F(PreferAsyncResourcesTest,
       UrchinAndSyncGoogleAnalyticsAboveOtherConentIsBad) {
  CreateScriptElement(kUrchinScriptUrl, body());
  CreateCssElement(body());
  CheckOneViolation(kRootUrl, kUrchinScriptUrl);
}

TEST_F(PreferAsyncResourcesTest, GoogleAnalyticsMixedResults) {
  CreateScriptElement(kGaScriptUrl, body());
  CreateScriptElement("http://test.com/test.js", body());
  CreateScriptElement(kUrchinScriptUrl, body());
  CheckTwoViolations(kRootUrl, kGaScriptUrl,
                     kRootUrl, kUrchinScriptUrl);
}

TEST_F(PreferAsyncResourcesTest, GoogleAnalyticsTwoViolations) {
  CreateScriptElement(kGaScriptUrl, body());
  CreateScriptElement(kUrchinScriptUrl, body());
  CreateScriptElement("http://test.com/test.js", body());
  CheckTwoViolations(kRootUrl, kGaScriptUrl,
                     kRootUrl, kUrchinScriptUrl);
}

TEST_F(PreferAsyncResourcesTest, FormatTest) {
  std::string expected =
      "The following resources are loaded synchronously. Load them "
      "asynchronously to reduce blocking of page rendering.\n"
      "  http://test.com/#foo loads http://www.google-analytics.com/ga.js "
      "synchronously.\n";
  CreateScriptElement(kGaScriptUrl, body());
  CreateCssElement(body());
  CheckFormattedOutput(expected);
}

TEST_F(PreferAsyncResourcesTest, FormatNoOutputTest) {
  CheckFormattedOutput("");
}

TEST_F(PreferAsyncResourcesTest, SyncGoogleAnalyticsInIframeIsBad) {
  FakeDomElement* iframe_root = CreateIframeElement(body());
  CreateScriptElement(kGaScriptUrl, iframe_root);
  CheckOneViolation(kIframeUrl, kGaScriptUrl);
}

TEST_F(PreferAsyncResourcesTest, SyncGoogleAnalyticsInIframeAboveCssIsBad) {
  FakeDomElement* iframe_root = CreateIframeElement(body());
  CreateScriptElement(kGaScriptUrl, iframe_root);
  CreateCssElement(iframe_root);
  CheckOneViolation(kIframeUrl, kGaScriptUrl);
}

TEST_F(PreferAsyncResourcesTest, SyncScriptInBodyAndIframeIsDoublyBad) {
  CreateScriptElement(kUrchinScriptUrl, body());
  FakeDomElement* iframe_root = CreateIframeElement(body());
  CreateScriptElement(kGaScriptUrl, iframe_root);
  CreateCssElement(iframe_root);
  CheckTwoViolations(kIframeUrl, kGaScriptUrl,
                     kRootUrl, kUrchinScriptUrl);
}

TEST_F(PreferAsyncResourcesTest, SyncFacebookBeforeAnyContentIsBad) {
  CreateScriptElement(kFacebookScriptEnUsUrl, body());
  CreateCssElement(body());
  CheckOneViolation(kRootUrl, kFacebookScriptEnUsUrl);
}

TEST_F(PreferAsyncResourcesTest, AsyncFacebookAnywhereIsGood) {
  FakeDomElement* element;
  NewScriptResource(kFacebookScriptEnUsUrl, body(), &element);
  element->AddAttribute("async", "");
  CreatePngElement(body());
  CreateCssElement(body());
  CreateScriptElement("http://test.com/test.js", body());
  CheckNoViolations();
}

TEST_F(PreferAsyncResourcesTest, SyncFacebookExtendedUrlIsOk) {
  CreateScriptElement("http://connect.facebook.net/en_US/all.jsfoo", body());
  CreateCssElement(body());
  CheckNoViolations();
}

TEST_F(PreferAsyncResourcesTest, SyncFacebookWithVersionIsBad) {
  const char* kUrl = "http://connect.facebook.net/en_US/all.js?v=25.9.51";
  CreateScriptElement(kUrl, body());
  CreateCssElement(body());
  CheckOneViolation(kRootUrl, kUrl);
}

TEST_F(PreferAsyncResourcesTest, SyncFacebookForAnyRegionIsBad) {
  CreateScriptElement(kFacebookScriptEnUsUrl, body());
  CreateScriptElement(kFacebookScriptEnGbUrl, body());
  CreateCssElement(body());
  CheckTwoViolations(kRootUrl, kFacebookScriptEnUsUrl,
                     kRootUrl, kFacebookScriptEnGbUrl);
}

TEST_F(PreferAsyncResourcesTest, FacebookUrlCornerCases) {
  CreateScriptElement(kFacebookScriptAcceptedUrl, body());
  CreateScriptElement(kFacebookScriptRejectedUrl, body());
  CheckOneViolation(kRootUrl, kFacebookScriptAcceptedUrl);
}

// Make sure the DOM traversal properly resolves relative URLs.
TEST_F(PreferAsyncResourcesRelativeTest, SyncGoogleAnalyticsRelativeUrl) {
  FakeDomElement* element;
  NewScriptResource(kGaScriptUrl, body(), &element);
  element->AddAttribute("src", "ga.js");
  CreateCssElement(body());
  CheckOneViolation(kRelativeRootUrl, kGaScriptUrl);
}

TEST_F(PreferAsyncResourcesTest, PlusOneAboveOtherContentIsBad) {
  CreateScriptElement(kPlusOneScriptUrl, body());
  CreateCssElement(body());
  CheckOneViolation(kRootUrl, kPlusOneScriptUrl);
}

TEST_F(PreferAsyncResourcesTest, TwitterOtherContentIsBad) {
  CreateScriptElement(kTwitterScriptUrl, body());
  CreateCssElement(body());
  CheckOneViolation(kRootUrl, kTwitterScriptUrl);
}

TEST_F(PreferAsyncResourcesTest, QuantCastOtherContentIsBad) {
  CreateScriptElement(kQuantCastScriptUrl, body());
  CreateCssElement(body());
  CheckOneViolation(kRootUrl, kQuantCastScriptUrl);
}

TEST_F(PreferAsyncResourcesTest, ComScoreOtherContentIsBad) {
  CreateScriptElement(kComScoreScriptUrl, body());
  CreateCssElement(body());
  CheckOneViolation(kRootUrl, kComScoreScriptUrl);
}

TEST_F(PreferAsyncResourcesTest, ComScoreSecureOtherContentIsBad) {
  CreateScriptElement(kComScoreSecureScriptUrl, body());
  CreateCssElement(body());
  CheckOneViolation(kRootUrl, kComScoreSecureScriptUrl);
}


TEST_F(PreferAsyncResourcesTest, GptOtherContentIsBad) {
  CreateScriptElement(kGptScriptUrl, body());
  CreateCssElement(body());
  CheckOneViolation(kRootUrl, kGptScriptUrl);
}

TEST_F(PreferAsyncResourcesTest, ShareThisOtherContentIsBad) {
  CreateScriptElement(kShareThisScriptUrl, body());
  CreateCssElement(body());
  CheckOneViolation(kRootUrl, kShareThisScriptUrl);
}

TEST_F(PreferAsyncResourcesTest, PinterestOtherContentIsBad) {
  CreateScriptElement(kPinterestScriptUrl, body());
  CreateCssElement(body());
  CheckOneViolation(kRootUrl, kPinterestScriptUrl);
}

TEST_F(PreferAsyncResourcesTest, DisqusOtherContentIsBad) {
  CreateScriptElement(kDisqusScriptUrl, body());
  CreateCssElement(body());
  CheckOneViolation(kRootUrl, kDisqusScriptUrl);
}

TEST_F(PreferAsyncResourcesTest, DisqusEmbedOtherContentIsBad) {
  CreateScriptElement(kDisqusEmbedScriptUrl, body());
  CreateCssElement(body());
  CheckOneViolation(kRootUrl, kDisqusEmbedScriptUrl);
}

TEST_F(PreferAsyncResourcesTest, ChartBeatOtherContentIsBad) {
  CreateScriptElement(kChartBeatScriptUrl, body());
  CreateCssElement(body());
  CheckOneViolation(kRootUrl, kChartBeatScriptUrl);
}

TEST_F(PreferAsyncResourcesTest, NewRelicOtherContentIsBad) {
  CreateScriptElement(kNewRelicScriptUrl, body());
  CreateCssElement(body());
  CheckOneViolation(kRootUrl, kNewRelicScriptUrl);
}

TEST_F(PreferAsyncResourcesTest, ClickyOtherContentIsBad) {
  CreateScriptElement(kClickyScriptUrl, body());
  CreateCssElement(body());
  CheckOneViolation(kRootUrl, kClickyScriptUrl);
}

TEST_F(PreferAsyncResourcesTest, BuySellAdsOtherContentIsBad) {
  CreateScriptElement(kBuySellAdsScriptUrl, body());
  CreateCssElement(body());
  CheckOneViolation(kRootUrl, kBuySellAdsScriptUrl);
}

TEST_F(PreferAsyncResourcesTest, StumbleUponOtherContentIsBad) {
  CreateScriptElement(kStumbleUponScriptUrl, body());
  CreateCssElement(body());
  CheckOneViolation(kRootUrl, kStumbleUponScriptUrl);
}

TEST_F(PreferAsyncResourcesTest, YandexOtherContentIsBad) {
  CreateScriptElement(kYandexScriptUrl, body());
  CreateCssElement(body());
  CheckOneViolation(kRootUrl, kYandexScriptUrl);
}

TEST_F(PreferAsyncResourcesTest, TyntOtherContentIsBad) {
  CreateScriptElement(kTyntScriptUrl, body());
  CreateCssElement(body());
  CheckOneViolation(kRootUrl, kTyntScriptUrl);
}

TEST_F(PreferAsyncResourcesTest, AdSenseShowAds1) {
  CreateScriptElement(kAdSenseShowAdsUrl1, body());
  CreateCssElement(body());
  CheckOneViolation(kRootUrl, kAdSenseShowAdsUrl1);
}

TEST_F(PreferAsyncResourcesTest, AdSenseShowAds2) {
  CreateScriptElement(kAdSenseShowAdsUrl2, body());
  CreateCssElement(body());
  CheckOneViolation(kRootUrl, kAdSenseShowAdsUrl2);
}

}  // namespace
