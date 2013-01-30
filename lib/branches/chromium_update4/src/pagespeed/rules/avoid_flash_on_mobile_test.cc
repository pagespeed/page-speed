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

#include "base/stringprintf.h"
#include "pagespeed/core/pagespeed_input.h"
#include "pagespeed/rules/avoid_flash_on_mobile.h"
#include "pagespeed/testing/pagespeed_test.h"

namespace {

using pagespeed::rules::AvoidFlashOnMobile;
using pagespeed::Resource;
using pagespeed_testing::FakeDomDocument;
using pagespeed_testing::FakeDomElement;
using pagespeed_testing::PagespeedRuleTest;

class AvoidFlashOnMobileTest : public PagespeedRuleTest<AvoidFlashOnMobile> {
 protected:
  static const char* kResultHeader;
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
};

const char* AvoidFlashOnMobileTest::kResultHeader =
    "The following %i Flash elements are included on the page or from "
    "included iframes. Adobe Flash Player is not supported on Apple iOS or "
    "Android versions greater than 4.0.x. Consider removing Flash objects "
    "and finding suitable replacements.";
const char* AvoidFlashOnMobileTest::kRootUrl = "http://example.com/";
const char* AvoidFlashOnMobileTest::kSwfUrl = "http://example.com/flash.swf";
const char* AvoidFlashOnMobileTest::kFlashMime =
    "application/x-shockwave-flash";
const char* AvoidFlashOnMobileTest::kFlashClassid =
    "clsid:d27cdb6e-ae6d-11cf-96b8-444553540000";
const int AvoidFlashOnMobileTest::kDefaultSize = 2000;

TEST_F(AvoidFlashOnMobileTest, EmptyDom) {
  CheckNoViolations();
}

TEST_F(AvoidFlashOnMobileTest, FlashEmbedSimple) {
  FakeDomElement* embed_element = FakeDomElement::New(body(), "embed");
  embed_element->AddAttribute("type", kFlashMime);
  embed_element->AddAttribute("src", kSwfUrl);
  std::string header = StringPrintf(kResultHeader, 1);
  std::string expected = header + "\n  " + kSwfUrl + "\n";
  CheckFormattedOutput(expected);
}

TEST_F(AvoidFlashOnMobileTest, FlashEmbedSize) {
  FakeDomElement* embed_element = FakeDomElement::New(body(), "embed");
  embed_element->AddAttribute("type", kFlashMime);
  embed_element->AddAttribute("src", kSwfUrl);
  embed_element->AddAttribute("width", "400px");
  embed_element->AddAttribute("height", "800px");
  std::string header = StringPrintf(kResultHeader, 1);
  std::string expected = header + "\n  " + kSwfUrl + " (400px x 800px)\n";
  CheckFormattedOutput(expected);
}

TEST_F(AvoidFlashOnMobileTest, FlashObjectSimple) {
  FakeDomElement* object_element = FakeDomElement::New(body(), "object");
  object_element->AddAttribute("type", kFlashMime);
  object_element->AddAttribute("data", kSwfUrl);
  std::string header = StringPrintf(kResultHeader, 1);
  std::string expected = header + "\n  " + kSwfUrl + "\n";
  CheckFormattedOutput(expected);
}

TEST_F(AvoidFlashOnMobileTest, FlashObjectSize) {
  FakeDomElement* object_element = FakeDomElement::New(body(), "object");
  object_element->AddAttribute("type", kFlashMime);
  object_element->AddAttribute("data", kSwfUrl);
  object_element->AddAttribute("width", "400");
  object_element->AddAttribute("height", "800");
  std::string header = StringPrintf(kResultHeader, 1);
  std::string expected = header + "\n  " + kSwfUrl + " (400 x 800)\n";
  CheckFormattedOutput(expected);
}

TEST_F(AvoidFlashOnMobileTest, FlashEmbedAndObject) {
  FakeDomElement* embed_element = FakeDomElement::New(body(), "embed");
  embed_element->AddAttribute("type", kFlashMime);
  embed_element->AddAttribute("src", "a.swf");
  embed_element->AddAttribute("width", "400px");
  embed_element->AddAttribute("height", "800px");
  FakeDomElement* object_element = FakeDomElement::New(body(), "object");
  object_element->AddAttribute("type", kFlashMime);
  object_element->AddAttribute("data", "b.swf");
  std::string header = StringPrintf(kResultHeader, 2);
  std::string expected = header + "\n"
      + "  http://example.com/a.swf (400px x 800px)\n"
      + "  http://example.com/b.swf\n";
  CheckFormattedOutput(expected);
}

TEST_F(AvoidFlashOnMobileTest, FlashActiveXObject) {
  FakeDomElement* object_element = FakeDomElement::New(body(), "object");
  object_element->AddAttribute("classid", kFlashClassid);
  FakeDomElement* param_name = FakeDomElement::New(object_element, "param");
  param_name->AddAttribute("name", "movie");
  param_name->AddAttribute("value", "flash.swf");
  CheckOneUrlViolation(kSwfUrl);
}

TEST_F(AvoidFlashOnMobileTest, FlashEmbedNoTypeNoResource) {
  FakeDomElement* embed_element = FakeDomElement::New(body(), "embed");
  embed_element->AddAttribute("src", "http://example.com/flash.SWF?q=1#a");
  CheckOneUrlViolation("http://example.com/flash.SWF?q=1");
}

TEST_F(AvoidFlashOnMobileTest, FlashEmbedNoTypeHasResource) {
  FakeDomElement* embed_element = FakeDomElement::New(body(), "embed");
  embed_element->AddAttribute("src", "movie");
  AddFlashResource("http://example.com/movie");
  CheckOneUrlViolation("http://example.com/movie");
}

TEST_F(AvoidFlashOnMobileTest, UnknownEmbedNoTypeNoResource) {
  FakeDomElement* embed_element = FakeDomElement::New(body(), "embed");
  embed_element->AddAttribute("src", "http://example.com/movie");
  // Cannot determine that the resource is flash, no violation
  CheckNoViolations();
}

TEST_F(AvoidFlashOnMobileTest, PngObjectNoTypeHasResource) {
  FakeDomElement* object_element = FakeDomElement::New(body(), "object");
  object_element->AddAttribute("data", kSwfUrl);
  AddTestResource(kSwfUrl, "image/png", kDefaultSize);
  // The mimetype image/png (not flash) should win over the filename (.swf)
  CheckNoViolations();
}

TEST_F(AvoidFlashOnMobileTest, FlashObjectNoType) {
  FakeDomElement* object_element = FakeDomElement::New(body(), "object");
  object_element->AddAttribute("data", kSwfUrl);
  CheckOneUrlViolation(kSwfUrl);
}

TEST_F(AvoidFlashOnMobileTest, FlashObjectTypeMovieNoData) {
  FakeDomElement* object_element = FakeDomElement::New(body(), "object");
  object_element->AddAttribute("type", kFlashMime);
  FakeDomElement* param_name = FakeDomElement::New(object_element, "param");
  param_name->AddAttribute("name", "movie");
  param_name->AddAttribute("value", kSwfUrl);
  CheckOneUrlViolation(kSwfUrl);
}

TEST_F(AvoidFlashOnMobileTest, FlashObjectMimeCase) {
  FakeDomElement* object_element = FakeDomElement::New(body(), "object");
  object_element->AddAttribute("type", "ApPlIcAtIoN/x-shockWAVE-FLASH");
  object_element->AddAttribute("data", kSwfUrl);
  CheckOneUrlViolation(kSwfUrl);
}

TEST_F(AvoidFlashOnMobileTest, FlashActiveXObjectClassidCase) {
  FakeDomElement* object_element = FakeDomElement::New(body(), "object");
  object_element->AddAttribute("classid",
                               "CLSID:D27CDB6E-AE6D-11CF-96B8-444553540000");
  FakeDomElement* param_name = FakeDomElement::New(object_element, "param");
  param_name->AddAttribute("name", "movie");
  param_name->AddAttribute("value", "flash.swf");

  CheckOneUrlViolation(kSwfUrl);
}

TEST_F(AvoidFlashOnMobileTest, FlashObjectInIFrame) {
  FakeDomElement* iframe = FakeDomElement::NewIframe(body());
  FakeDomDocument* iframe_doc;
  NewDocumentResource("http://example.com/frame/i.html", iframe, &iframe_doc);
  FakeDomElement* html2 = FakeDomElement::NewRoot(iframe_doc, "html");
  FakeDomElement* object_element = FakeDomElement::New(html2, "object");
  object_element->AddAttribute("type", "application/x-shockwave-flash");
  // Make the data attribute relative.
  object_element->AddAttribute("data", "flash.swf");
  CheckOneUrlViolation("http://example.com/frame/flash.swf");
}

TEST_F(AvoidFlashOnMobileTest, AdobeTwiceCooked) {
  // http://helpx.adobe.com/flash/kb/object-tag-syntax-flash-professional.html
  FakeDomElement* object_element = FakeDomElement::New(body(), "object");
  object_element->AddAttribute("classid",
                               "clsid:d27cdb6e-ae6d-11cf-96b8-444553540000");
  object_element->AddAttribute("width", "550");
  object_element->AddAttribute("height", "400");
  object_element->AddAttribute("id", "movie_name");
  object_element->AddAttribute("align", "middle");
  FakeDomElement* param_name = FakeDomElement::New(object_element, "param");
  param_name->AddAttribute("name", "movie");
  param_name->AddAttribute("value", "flash.swf");
  FakeDomElement* no_ie_object = FakeDomElement::New(object_element, "object");
  no_ie_object->AddAttribute("type", "application/x-shockwave-flash");
  no_ie_object->AddAttribute("data", "flash.swf");
  no_ie_object->AddAttribute("width", "550");
  no_ie_object->AddAttribute("height", "400");
  FakeDomElement* no_ie_name = FakeDomElement::New(no_ie_object, "param");
  no_ie_name->AddAttribute("name", "movie");
  no_ie_name->AddAttribute("value", "flash.swf");
  FakeDomElement* getflash = FakeDomElement::New(no_ie_object, "a");
  getflash->AddAttribute("href", "http://www.adobe.com/go/getflash");
  CheckOneUrlViolation(kSwfUrl);
}

TEST_F(AvoidFlashOnMobileTest, FlashSatay) {
  // http://www.alistapart.com/articles/flashsatay
  FakeDomElement* object_element = FakeDomElement::New(body(), "object");
  object_element->AddAttribute("type", "application/x-shockwave-flash");
  object_element->AddAttribute("data", "c.swf?path=movie.swf");
  object_element->AddAttribute("width", "400");
  object_element->AddAttribute("height", "300");
  FakeDomElement* param_name = FakeDomElement::New(object_element, "param");
  param_name->AddAttribute("name", "movie");
  param_name->AddAttribute("value", "c.swf?path=movie.swf");
  CheckOneUrlViolation("http://example.com/c.swf?path=movie.swf");
}

TEST_F(AvoidFlashOnMobileTest, SilverlightObject) {
  // http://msdn.microsoft.com/en-us/library/cc189089(v=vs.95).aspx
  FakeDomElement* object_element = FakeDomElement::New(body(), "object");
  object_element->AddAttribute("width", "300");
  object_element->AddAttribute("height", "300");
  object_element->AddAttribute("data", "data:application/x-silverlight-2,");
  object_element->AddAttribute("type", "application/x-silverlight-2");
  FakeDomElement* param_element = FakeDomElement::New(object_element, "param");
  param_element->AddAttribute("name", "source");
  param_element->AddAttribute("value", "SilverlightApplication1.xap");
  // Only testing for Adobe Flash
  CheckNoViolations();
}

}  // namespace
