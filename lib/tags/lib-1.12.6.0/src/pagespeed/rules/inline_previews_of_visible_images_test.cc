// Copyright 2011 Google Inc. All Rights Reserved.
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
#include "pagespeed/proto/pagespeed_output.pb.h"
#include "pagespeed/rules/inline_previews_of_visible_images.h"
#include "pagespeed/testing/pagespeed_test.h"

using pagespeed::rules::InlinePreviewsOfVisibleImages;
using pagespeed_testing::FakeDomDocument;
using pagespeed_testing::FakeDomElement;
using pagespeed_testing::PagespeedRuleTest;

namespace {

class InlinePreviewsOfVisibleImagesTest
    : public PagespeedRuleTest<InlinePreviewsOfVisibleImages> {
 protected:
  static const char* kRootUrl;
  static const char* kIframeUrl;
  static const char* kImg1Url;
  static const char* kImg2Url;
  static const char* kAboveTheFoldUrl;

  static const int kEarlyResourceLoadTimeMillis;
  static const int kOnloadMillis;

  virtual void DoSetUp() {
    SetViewportWidthAndHeight(1024, 768);
    NewPrimaryResource(kRootUrl);
    CreateHtmlHeadBodyElements();
    SetOnloadTimeMillis(kOnloadMillis);
  }

  void AddImage(const char* url, FakeDomElement* parent,
                int x, int y, int width, int height,
                int request_start_time_millis) {
    FakeDomElement* img = NULL;
    NewPngResource(url, parent, &img)->SetRequestStartTimeMillis(
        request_start_time_millis);
    img->SetCoordinates(x, y);
    img->SetActualWidthAndHeight(width, height);
  }

  void AddImage(const char* url, int x, int y, int width, int height,
                int request_start_time_millis) {
    AddImage(url, body(), x, y, width, height, request_start_time_millis);
  }

  void AddVisibleImage(int request_start_time_millis) {
    AddImage(kAboveTheFoldUrl, 5, 5, 10, 10, request_start_time_millis);
  }
};

const char* InlinePreviewsOfVisibleImagesTest::kRootUrl = "http://test.com/";
const char* InlinePreviewsOfVisibleImagesTest::kIframeUrl =
    "http://test.com/frame.html";
const char* InlinePreviewsOfVisibleImagesTest::kImg1Url =
    "http://test.com/a.png";
const char* InlinePreviewsOfVisibleImagesTest::kImg2Url =
    "http://test.com/b.png";
const char* InlinePreviewsOfVisibleImagesTest::kAboveTheFoldUrl =
    "http://test.com/atf.png";
const int InlinePreviewsOfVisibleImagesTest::kOnloadMillis = 100;
const int InlinePreviewsOfVisibleImagesTest::kEarlyResourceLoadTimeMillis = 1;

TEST_F(InlinePreviewsOfVisibleImagesTest, EmptyDom) {
  CheckNoViolations();
}

TEST_F(InlinePreviewsOfVisibleImagesTest, ImageMissingDimensions) {
  FakeDomElement* img1 = NULL;
  NewPngResource(kImg1Url, body(), &img1);
  CheckNoViolations();
}

TEST_F(InlinePreviewsOfVisibleImagesTest, ImageAboveTheFold) {
  AddImage(kImg1Url, 0, 0, 10, 10, kEarlyResourceLoadTimeMillis);
  CheckOneUrlViolation(kImg1Url);
}

TEST_F(InlinePreviewsOfVisibleImagesTest, ImageAboveTheFoldNoWidth) {
  AddImage(kImg1Url, 0, 0, 0, 10, kEarlyResourceLoadTimeMillis);
  CheckNoViolations();
}

TEST_F(InlinePreviewsOfVisibleImagesTest, ImageBelowTheFold) {
  AddImage(kImg1Url, 0, 768, 10, 10, kEarlyResourceLoadTimeMillis);
  CheckNoViolations();
}

TEST_F(InlinePreviewsOfVisibleImagesTest, TwoImagesAboveTheFold) {
  AddImage(kImg1Url, 0, 100, 10, 10, kEarlyResourceLoadTimeMillis);
  AddImage(kImg2Url, 0, 200, 10, 10, kEarlyResourceLoadTimeMillis);
  CheckTwoUrlViolations(kImg1Url, kImg2Url);
}

TEST_F(InlinePreviewsOfVisibleImagesTest, ImageOverlappingTheFold) {
  AddImage(kImg1Url, 0, 760, 10, 10, kEarlyResourceLoadTimeMillis);
  CheckOneUrlViolation(kImg1Url);
}

TEST_F(InlinePreviewsOfVisibleImagesTest, SameImageAboveAndBelowTheFold) {
  AddImage(kImg1Url, 0, 768, 10, 10, kEarlyResourceLoadTimeMillis);

  FakeDomElement* img2 = FakeDomElement::NewImg(body(), kImg1Url);
  img2->SetCoordinates(0, 0);
  img2->SetActualWidthAndHeight(10, 10);

  CheckOneUrlViolation(kImg1Url);
}

TEST_F(InlinePreviewsOfVisibleImagesTest, OneImageVisibleOneNotVisible) {
  AddImage(kImg1Url, 1024, 100, 10, 10, kEarlyResourceLoadTimeMillis);
  AddImage(kImg2Url, 100, 100, 10, 10, kEarlyResourceLoadTimeMillis);
  CheckOneUrlViolation(kImg2Url);
}

TEST_F(InlinePreviewsOfVisibleImagesTest, ImageBelowTheFoldAfterOnload) {
  AddImage(kImg1Url, 1024, 100, 10, 10, kOnloadMillis + 1);
  CheckNoViolations();
}

TEST_F(InlinePreviewsOfVisibleImagesTest, RedirectedImage) {
  FakeDomElement* img1 = NULL;
  NewRedirectedPngResource(
      kImg1Url, kImg2Url, body(), &img1)->SetRequestStartTimeMillis(
          kEarlyResourceLoadTimeMillis);
  img1->SetCoordinates(100, 100);
  img1->SetActualWidthAndHeight(10, 10);
  CheckOneUrlViolation(kImg2Url);
}

TEST_F(InlinePreviewsOfVisibleImagesTest, ImageInIframe) {
  FakeDomElement* iframe = FakeDomElement::NewIframe(body());
  iframe->SetCoordinates(200, 200);
  iframe->SetActualWidthAndHeight(200, 200);
  FakeDomDocument* iframe_doc = NULL;
  NewDocumentResource(kIframeUrl, iframe, &iframe_doc);
  FakeDomElement* html = FakeDomElement::NewRoot(iframe_doc, "html");

  // 200, 200 + 0, 0 = 200, 200, which is above the fold.
  AddImage(kImg1Url, html, 0, 0, 10, 10, kEarlyResourceLoadTimeMillis);

  // 200, 200 + 0, 700 = 200, 900, which is below the fold.
  AddImage(kImg2Url, html, 0, 700, 10, 10, kEarlyResourceLoadTimeMillis);

  CheckOneUrlViolation(kImg1Url);
}

}  // namespace
