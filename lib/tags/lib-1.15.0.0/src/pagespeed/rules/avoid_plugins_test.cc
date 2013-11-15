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

#include "pagespeed/core/pagespeed_input.h"
#include "pagespeed/rules/avoid_plugins.h"
#include "pagespeed/testing/pagespeed_test.h"

namespace {

using pagespeed::rules::AvoidPlugins;
using pagespeed::Resource;
using pagespeed::Rule;
using pagespeed_testing::FakeDomDocument;
using pagespeed_testing::FakeDomElement;
using pagespeed_testing::PagespeedRuleTest;

const char* kPassingSummary =
    "Your page does not appear to use plugins, which would prevent "
    "content from being usable on many platforms. Learn more about the "
    "importance of avoiding plugins"
    "<https://developers.google.com/speed/docs/insights/AvoidPlugins>.\n";
const char* kSummary =
    "Your page uses plugins, which prevents portions of your page from "
    "being used on many platforms. Find alternatives for plugin based content"
    "<https://developers.google.com/speed/docs/insights/AvoidPlugins> "
    "to increase compatibility.\n";
const char* kFlashBlock =
    "Find alternatives for the following Flash plugins.\n";
const char* kSilverlightBlock =
    "Find alternatives for the following Silverlight plugins.\n";
const char* kJavaBlock =
    "Find alternatives for the following Java plugins.\n";
const char* kUnknownBlock =
    "Find alternatives for the following plugins.\n";

class AvoidPluginsTest : public PagespeedRuleTest<AvoidPlugins> {
 protected:
  static const char* kRootUrl;
  static const char* kSwfUrl;
  static const char* kFlashMime;
  static const char* kFlashClassid;
  static const int kDefaultSize;

  void AddTestResource(const char* url, const char* content_type,
                       const int size) {
    Resource* resource = New200Resource(url);
    resource->AddResponseHeader("Content-Type", content_type);
    std::string body(size, '.');
    resource->SetResponseBody(body);
  }

  void AddFlashResource(const char* url) {
    AddTestResource(url, kFlashMime, kDefaultSize);
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

  void CheckOneUrl(const std::string& block, const std::string url) {
    std::string expected = std::string() + kSummary + block + "  " + url + "\n";
    CheckFormattedOutput(expected);
    EXPECT_GT(ComputeRuleImpact(), 0);
  }

  void CheckOneUnknownUrl(const std::string url, const std::string mime) {
    std::string expected = std::string() + kSummary + kUnknownBlock +
        "  " + url + " (" + mime + ").\n";
    CheckFormattedOutput(expected);
    EXPECT_GT(ComputeRuleImpact(), 0);
  }

  void CheckOneUnknownPluginNoUrl(const std::string& identifier) {
    std::string expected = std::string() + kSummary + kUnknownBlock +
        "  Unknown plugin of type " + identifier + ".\n";
    CheckFormattedOutput(expected);
    EXPECT_GT(ComputeRuleImpact(), 0);
  }
};

const char* AvoidPluginsTest::kRootUrl = "http://example.com/";
const char* AvoidPluginsTest::kSwfUrl = "http://example.com/flash.swf";
const char* AvoidPluginsTest::kFlashMime =
    "application/x-shockwave-flash";
const char* AvoidPluginsTest::kFlashClassid =
    "clsid:d27cdb6e-ae6d-11cf-96b8-444553540000";
const int AvoidPluginsTest::kDefaultSize = 2000;

TEST_F(AvoidPluginsTest, EmptyDom) {
  CheckFormattedOutput(kPassingSummary);
  EXPECT_EQ(0, ComputeRuleImpact());
}

TEST_F(AvoidPluginsTest, FlashEmbedSimple) {
  FakeDomElement* embed = FakeDomElement::New(body(), "embed");
  embed->AddAttribute("type", kFlashMime);
  embed->AddAttribute("src", "flash");
  CheckOneUrl(kFlashBlock, "http://example.com/flash");
  // No layout information, score as medium impact.
  EXPECT_EQ(Rule::kImpactMediumCutoff, ComputeRuleImpact());
}

TEST_F(AvoidPluginsTest, FlashEmbed20PercentOfSize) {
  SetViewportWidthAndHeight(100, 100);  // 10000 px
  FakeDomElement* embed = FakeDomElement::New(body(), "embed");
  embed->AddAttribute("type", kFlashMime);
  embed->AddAttribute("src", kSwfUrl);
  embed->SetCoordinates(5, 5);
  embed->SetActualWidthAndHeight(50, 40);  // 2000 px
  std::string expected =
      std::string() + kSummary + kFlashBlock +
      "  " + kSwfUrl + " (50 x 40) final[5,5,50,40].\n";
  CheckFormattedOutput(expected);
  // One plugin that's 20% of the ATF should be a high impact result.
  EXPECT_EQ(Rule::kImpactHighCutoff, ComputeRuleImpact());
}

TEST_F(AvoidPluginsTest, TwoClippedFlashEmbed20PercentOfSize) {
  SetViewportWidthAndHeight(100, 100);
  FakeDomElement* embed = FakeDomElement::New(body(), "embed");
  embed->AddAttribute("type", kFlashMime);
  embed->AddAttribute("src", kSwfUrl);
  embed->SetCoordinates(0, 0);
  embed->SetActualWidthAndHeight(50, 40);
  FakeDomElement* embed_2 = FakeDomElement::New(body(), "embed");
  embed_2->AddAttribute("type", kFlashMime);
  embed_2->AddAttribute("src", kSwfUrl);
  embed_2->SetCoordinates(100 - 50, 100 - 40);
  embed_2->SetActualWidthAndHeight(800, 900);
  std::string expected =
      std::string() + kSummary + kFlashBlock +
      "  " + kSwfUrl + " (50 x 40) final[0,0,50,40].\n" +
      "  " + kSwfUrl + " (800 x 900) final[50,60,800,900].\n";
  CheckFormattedOutput(expected);
  // Each plugin is 20% of the ATF viewport after clipping.
  EXPECT_EQ(2 * Rule::kImpactHighCutoff, ComputeRuleImpact());
}

TEST_F(AvoidPluginsTest, FlashEmbedSize) {
  SetViewportWidthAndHeight(1024, 768);
  FakeDomElement* embed = FakeDomElement::New(body(), "embed");
  embed->AddAttribute("type", kFlashMime);
  embed->AddAttribute("src", kSwfUrl);
  embed->SetCoordinates(11, 22);
  embed->SetActualWidthAndHeight(400, 300);
  std::string expected =
      std::string() + kSummary + kFlashBlock +
      "  " + kSwfUrl + " (400 x 300) final[11,22,400,300].\n";
  CheckFormattedOutput(expected);
  EXPECT_NEAR(3.0 + ((10.0 - 3.0) / 0.2) * ((400.0*300.0) / (1024.0*768.0)),
              ComputeRuleImpact(), 0.01);
}

TEST_F(AvoidPluginsTest, FlashObjectSimple) {
  FakeDomElement* object = FakeDomElement::New(body(), "object");
  object->AddAttribute("type", kFlashMime);
  object->AddAttribute("data", kSwfUrl);
  CheckOneUrl(kFlashBlock, kSwfUrl);
}

TEST_F(AvoidPluginsTest, FlashObjectSize) {
  FakeDomElement* object = FakeDomElement::New(body(), "object");
  object->AddAttribute("type", kFlashMime);
  object->AddAttribute("data", kSwfUrl);
  object->SetCoordinates(111, 222);
  object->SetActualWidthAndHeight(400, 800);
  std::string expected =
      std::string() + kSummary + kFlashBlock +
      "  " + kSwfUrl + " (400 x 800) final[111,222,400,800].\n";
  CheckFormattedOutput(expected);
}

TEST_F(AvoidPluginsTest, FlashEmbedAndObject) {
  FakeDomElement* embed = FakeDomElement::New(body(), "embed");
  embed->AddAttribute("type", kFlashMime);
  embed->AddAttribute("src", "a.swf");
  embed->SetCoordinates(111, 222);
  embed->SetActualWidthAndHeight(400, 800);
  FakeDomElement* object = FakeDomElement::New(body(), "object");
  object->AddAttribute("type", kFlashMime);
  object->AddAttribute("data", "b.swf");
  std::string expected =
      std::string() + kSummary + kFlashBlock +
      "  http://example.com/a.swf (400 x 800) final[111,222,400,800].\n" +
      "  http://example.com/b.swf\n";
  CheckFormattedOutput(expected);
  // Since a viewport wasn't provided, this should be scored as two plugins
  // with unknown area.
  EXPECT_EQ(6.0, ComputeRuleImpact());
}

TEST_F(AvoidPluginsTest, FlashActiveXObject) {
  FakeDomElement* object = FakeDomElement::New(body(), "object");
  object->AddAttribute("classid", kFlashClassid);
  FakeDomElement* param_name = FakeDomElement::New(object, "param");
  param_name->AddAttribute("name", "movie");
  param_name->AddAttribute("value", "flash.swf");
  CheckOneUrl(kFlashBlock, kSwfUrl);
}

TEST_F(AvoidPluginsTest, FlashEmbedNoTypeNoResource) {
  FakeDomElement* embed = FakeDomElement::New(body(), "embed");
  embed->AddAttribute("src", "http://example.com/flash.SWF?q=1#a");
  CheckOneUrl(kFlashBlock, "http://example.com/flash.SWF?q=1");
}

TEST_F(AvoidPluginsTest, FlashEmbedNoTypeHasResource) {
  FakeDomElement* embed = FakeDomElement::New(body(), "embed");
  embed->AddAttribute("src", "movie");
  AddFlashResource("http://example.com/movie");
  CheckOneUrl(kFlashBlock, "http://example.com/movie");
}

TEST_F(AvoidPluginsTest, UnknownEmbedNoTypeNoResource) {
  FakeDomElement* embed = FakeDomElement::New(body(), "embed");
  embed->AddAttribute("src", "http://example.com/movie");
  CheckOneUrl(kUnknownBlock, "http://example.com/movie");
}

TEST_F(AvoidPluginsTest, ObjectNoTypeHasPassingResource) {
  FakeDomElement* object = FakeDomElement::New(body(), "object");
  object->AddAttribute("data", kSwfUrl);
  AddTestResource(kSwfUrl, "image/png", kDefaultSize);
  // The mimetype image/png (not flash) should win over the filename (.swf).
  // We allow image/*, so this should pass.
  CheckNoViolations();
  EXPECT_EQ(0, ComputeRuleImpact());
}

TEST_F(AvoidPluginsTest, FlashObjectNoType) {
  FakeDomElement* object = FakeDomElement::New(body(), "object");
  object->AddAttribute("data", kSwfUrl);
  CheckOneUrl(kFlashBlock, kSwfUrl);
}

TEST_F(AvoidPluginsTest, FlashObjectTypeMovieNoData) {
  FakeDomElement* object = FakeDomElement::New(body(), "object");
  object->AddAttribute("type", kFlashMime);
  FakeDomElement* param_name = FakeDomElement::New(object, "param");
  param_name->AddAttribute("name", "movie");
  param_name->AddAttribute("value", kSwfUrl);
  CheckOneUrl(kFlashBlock, kSwfUrl);
}

TEST_F(AvoidPluginsTest, FlashObjectMimeCase) {
  FakeDomElement* object = FakeDomElement::New(body(), "object");
  object->AddAttribute("type", "ApPlIcAtIoN/x-shockWAVE-FLASH");
  object->AddAttribute("data", kSwfUrl);
  CheckOneUrl(kFlashBlock, kSwfUrl);
}

TEST_F(AvoidPluginsTest, FlashActiveXObjectClassidCase) {
  FakeDomElement* object = FakeDomElement::New(body(), "object");
  object->AddAttribute("classid",
                               "CLSID:D27CDB6E-AE6D-11CF-96B8-444553540000");
  FakeDomElement* param_name = FakeDomElement::New(object, "param");
  param_name->AddAttribute("name", "movie");
  param_name->AddAttribute("value", "flash.swf");
  CheckOneUrl(kFlashBlock, kSwfUrl);
}

TEST_F(AvoidPluginsTest, FlashObjectInIFrame) {
  FakeDomElement* iframe = FakeDomElement::NewIframe(body());
  FakeDomDocument* iframe_doc;
  NewDocumentResource("http://example.com/frame/i.html", iframe, &iframe_doc);
  FakeDomElement* html2 = FakeDomElement::NewRoot(iframe_doc, "html");
  FakeDomElement* object = FakeDomElement::New(html2, "object");
  object->AddAttribute("type", "application/x-shockwave-flash");
  // Make the data attribute relative.
  object->AddAttribute("data", "flash.swf");
  CheckOneUrl(kFlashBlock, "http://example.com/frame/flash.swf");
}

TEST_F(AvoidPluginsTest, AdobeTwiceCooked) {
  // http://helpx.adobe.com/flash/kb/object-tag-syntax-flash-professional.html
  FakeDomElement* object = FakeDomElement::New(body(), "object");
  object->AddAttribute("classid",
                               "clsid:d27cdb6e-ae6d-11cf-96b8-444553540000");
  object->AddAttribute("width", "550");
  object->AddAttribute("height", "400");
  object->AddAttribute("id", "movie_name");
  object->AddAttribute("align", "middle");
  FakeDomElement* param_name = FakeDomElement::New(object, "param");
  param_name->AddAttribute("name", "movie");
  param_name->AddAttribute("value", "flash.swf");
  FakeDomElement* no_ie_object = FakeDomElement::New(object, "object");
  no_ie_object->AddAttribute("type", "application/x-shockwave-flash");
  no_ie_object->AddAttribute("data", "flash.swf");
  no_ie_object->AddAttribute("width", "550");
  no_ie_object->AddAttribute("height", "400");
  FakeDomElement* no_ie_name = FakeDomElement::New(no_ie_object, "param");
  no_ie_name->AddAttribute("name", "movie");
  no_ie_name->AddAttribute("value", "flash.swf");
  FakeDomElement* getflash = FakeDomElement::New(no_ie_object, "a");
  getflash->AddAttribute("href", "http://www.adobe.com/go/getflash");
  CheckOneUrl(kFlashBlock, kSwfUrl);
}

TEST_F(AvoidPluginsTest, FlashSatay) {
  // http://www.alistapart.com/articles/flashsatay
  FakeDomElement* object = FakeDomElement::New(body(), "object");
  object->AddAttribute("type", "application/x-shockwave-flash");
  object->AddAttribute("data", "c.swf?path=movie.swf");
  object->AddAttribute("width", "400");
  object->AddAttribute("height", "300");
  FakeDomElement* param_name = FakeDomElement::New(object, "param");
  param_name->AddAttribute("name", "movie");
  param_name->AddAttribute("value", "c.swf?path=movie.swf");
  CheckOneUrl(kFlashBlock, "http://example.com/c.swf?path=movie.swf");
}

TEST_F(AvoidPluginsTest, SilverlightObject) {
  // http://www.microsoft.com/en-us/download/details.aspx?id=15072
  FakeDomElement* object = FakeDomElement::New(body(), "object");
  object->AddAttribute("data", "data:application/x-silverlight,");
  object->AddAttribute("type", "application/x-silverlight");
  object->AddAttribute("width", "100%");
  object->AddAttribute("height", "100%");
  FakeDomElement* param = FakeDomElement::New(object, "param");
  param->AddAttribute("name", "source");
  param->AddAttribute("value", "app/InstallUXTest.xap");
  FakeDomElement* param_2 =
      FakeDomElement::New(object, "param");
  param_2->AddAttribute("name", "background");
  param_2->AddAttribute("value", "white");
  FakeDomElement::New(object, "div");
  CheckOneUrl(kSilverlightBlock, "http://example.com/app/InstallUXTest.xap");
}

TEST_F(AvoidPluginsTest, Silverlight2Object) {
  // http://msdn.microsoft.com/en-us/library/cc189089(v=vs.95).aspx
  FakeDomElement* object = FakeDomElement::New(body(), "object");
  object->AddAttribute("width", "300");
  object->AddAttribute("height", "300");
  object->AddAttribute("data", "data:application/x-silverlight-2,");
  object->AddAttribute("type", "application/x-silverlight-2");
  FakeDomElement* param = FakeDomElement::New(object, "param");
  param->AddAttribute("name", "source");
  param->AddAttribute("value", "SilverlightApplication1.xap");
  CheckOneUrl(kSilverlightBlock,
              "http://example.com/SilverlightApplication1.xap");
}

TEST_F(AvoidPluginsTest, JavaApplet) {
  // http://docs.oracle.com/javase/1.5.0/docs/guide/plugin/developer_guide/using_tags.html
  FakeDomElement* applet = FakeDomElement::New(body(), "applet");
  applet->AddAttribute("code", "Applet1.class");
  applet->AddAttribute("width", "200");
  applet->AddAttribute("height", "200");
  CheckOneUrl(kJavaBlock, "http://example.com/Applet1.class");
}

TEST_F(AvoidPluginsTest, JavaObjectLatest) {
  // http://docs.oracle.com/javase/1.5.0/docs/guide/plugin/developer_guide/using_tags.html
  FakeDomElement* object = FakeDomElement::New(body(), "object");
  // "This example instructs Internet Explorer to use the latest installed
  // version of Java Plug-in."
  object->AddAttribute("classid",
                               "clsid:8AD9C840-044E-11D1-B3E9-00805F499D93");
  FakeDomElement* param = FakeDomElement::New(object, "param");
  param->AddAttribute("name", "code");
  param->AddAttribute("value", "Applet1.class");
  CheckOneUrl(kJavaBlock, "http://example.com/Applet1.class");
}

TEST_F(AvoidPluginsTest, JavaObject1_5_0) {
  // http://docs.oracle.com/javase/1.5.0/docs/guide/plugin/developer_guide/using_tags.html
  FakeDomElement* object = FakeDomElement::New(body(), "object");
  // "Following is an alternative form of the classid attribute:
  //
  //   classid="clsid:CAFEEFAC-xxxx-yyyy-zzzz-ABCDEFFEDCBA"
  //
  // In this form, "xxxx", "yyyy", and "zzzz" are four-digit numbers that
  // identify the specific version of Java Plug-in to be used.
  //
  // For example, to use Java Plug-in version 1.5.0, you specify:"
  object->AddAttribute("classid",
                               "clsid:CAFEEFAC-0015-0000-0000-ABCDEFFEDCBA");
  FakeDomElement* param = FakeDomElement::New(object, "param");
  param->AddAttribute("name", "code");
  param->AddAttribute("value", "Applet1.class");
  CheckOneUrl(kJavaBlock, "http://example.com/Applet1.class");
}

TEST_F(AvoidPluginsTest, JavaEmbed) {
  // http://docs.oracle.com/javase/1.5.0/docs/guide/plugin/developer_guide/using_tags.html
  FakeDomElement* embed = FakeDomElement::New(body(), "embed");
  embed->AddAttribute("code", "Applet1.class");
  embed->AddAttribute("width", "200");
  embed->AddAttribute("height", "200");
  embed->AddAttribute("type",
                              "application/x-java-applet;version=1.5.0");
  embed->AddAttribute("pluginspage",
                              "http://java.sun.com/j2se/1.5.0/download.html");
  CheckOneUrl(kJavaBlock, "http://example.com/Applet1.class");
}

TEST_F(AvoidPluginsTest, JavaObjectCommentEmbed) {
  // http://docs.oracle.com/javase/1.5.0/docs/guide/plugin/developer_guide/using_tags.html
  FakeDomElement* object = FakeDomElement::New(body(), "object");
  object->AddAttribute("classid", "clsid:CAFEEFAC-0015-0000-0000-ABCDEFFEDCBA");
  FakeDomElement* param_code = FakeDomElement::New(object, "param");
  param_code->AddAttribute("name", "code");
  param_code->AddAttribute("value", "Applet1.class");
  FakeDomElement* comment = FakeDomElement::New(object, "comment");
  FakeDomElement* embed = FakeDomElement::New(comment, "embed");
  embed->AddAttribute("code", "Applet1.class");
  embed->AddAttribute("type", "application/x-java-applet;jpi-version=1.5.0");
  FakeDomElement::New(embed, "noembed");
  CheckOneUrl(kJavaBlock, "http://example.com/Applet1.class");
}

TEST_F(AvoidPluginsTest, JavaAppletClasspathArchiveList) {
  FakeDomElement* applet = FakeDomElement::New(body(), "applet");
  applet->AddAttribute("code", "com.example.applet.class");
  applet->AddAttribute("codebase", "./applets/folder/../folder2");
  applet->AddAttribute("archive", "MainJar.jar,library.jar,lib2.jar");
  applet->AddAttribute("width", "200");
  applet->AddAttribute("height", "200");
  CheckOneUrl(kJavaBlock, "http://example.com/applets/folder2/MainJar.jar");
}

TEST_F(AvoidPluginsTest, UnknownObjectWithType) {
  FakeDomElement* object = FakeDomElement::New(body(), "object");
  object->AddAttribute("type", "application/x-my-strange-plugin");
  object->AddAttribute("data", "plugin.wat");
  CheckOneUnknownUrl("http://example.com/plugin.wat",
                     "application/x-my-strange-plugin");
}

TEST_F(AvoidPluginsTest, UnknownObjectWithClassidNoData) {
  FakeDomElement* object = FakeDomElement::New(body(), "object");
  object->AddAttribute("classid", "clsid:AAAAAAAA-AAAA-AAAA-AAAA-AAAAAAAAAAAA");
  FakeDomElement* param_code = FakeDomElement::New(object, "param");
    param_code->AddAttribute("name", "sneaky-src-param");
    param_code->AddAttribute("value", "plugin.wat");
  CheckOneUnknownPluginNoUrl("clsid:AAAAAAAA-AAAA-AAAA-AAAA-AAAAAAAAAAAA");
}

TEST_F(AvoidPluginsTest, UnknownEmbedWithType) {
  FakeDomElement* embed = FakeDomElement::New(body(), "embed");
  embed->AddAttribute("type", "application/x-my-strange-plugin");
  embed->AddAttribute("src", "plugin.wat");
  CheckOneUnknownUrl("http://example.com/plugin.wat",
                     "application/x-my-strange-plugin");
}

TEST_F(AvoidPluginsTest, LotsOfPlugins) {
  {
    FakeDomElement* object = FakeDomElement::New(body(), "object");
    object->AddAttribute("data", "data:application/x-silverlight-2,");
    object->AddAttribute("type", "application/x-silverlight-2");
    object->SetCoordinates(20, 10);
    object->SetActualWidthAndHeight(200, 100);
    FakeDomElement* param = FakeDomElement::New(object, "param");
    param->AddAttribute("name", "source");
    param->AddAttribute("value", "sl1");
  }
  {
    FakeDomElement* object = FakeDomElement::New(body(), "object");
    object->AddAttribute("data", "http://other.com/flash.Swf");
    object->SetCoordinates(30, 120);
    object->SetActualWidthAndHeight(300, 80);
  }
  {
    FakeDomElement* applet = FakeDomElement::New(body(), "applet");
    applet->AddAttribute("code", "java.class");
    applet->AddAttribute("width", "200");
    applet->AddAttribute("height", "400");
    applet->SetCoordinates(250, 130);
    applet->SetActualWidthAndHeight(200, 400);
  }
  {
    FakeDomElement* embed = FakeDomElement::New(body(), "embed");
    embed->AddAttribute("type", "application/x-unknown");
    embed->AddAttribute("src", "plugin.wat");
    embed->SetCoordinates(25, 450);
    embed->SetActualWidthAndHeight(111, 222);
  }
  {
    FakeDomElement* object = FakeDomElement::New(body(), "object");
    object->AddAttribute("type", kFlashMime);
    object->SetCoordinates(2, 1000);
    object->SetActualWidthAndHeight(123, 456);
    FakeDomElement* param_name = FakeDomElement::New(object, "param");
    param_name->AddAttribute("name", "movie");
    param_name->AddAttribute("value", "fla2");
  }
  {
    FakeDomElement* embed = FakeDomElement::New(body(), "embed");
    embed->AddAttribute("type", "video/mp4");
    embed->AddAttribute("src", "should_ignore_this_video.mp4");
  }

  // Blocks should be sorted by the order the first instance was encountered on
  // the page, and URLs inside a block should be  sorted by the order on the
  // page.
  std::string expected =
      std::string() + kSummary +
      kSilverlightBlock +
      "  http://example.com/sl1 (200 x 100) final[20,10,200,100].\n" +
      kFlashBlock +
      "  http://other.com/flash.Swf (300 x 80) final[30,120,300,80].\n" +
      "  http://example.com/fla2 (123 x 456) final[2,1000,123,456].\n" +
      kJavaBlock +
      "  http://example.com/java.class (200 x 400) final[250,130,200,400].\n" +
      kUnknownBlock +
      "  http://example.com/plugin.wat (application/x-unknown: 111 x 222) "
      "final[25,450,111,222].\n";
  CheckFormattedOutput(expected);
  EXPECT_GT(ComputeRuleImpact(), 0);
}

}  // namespace
