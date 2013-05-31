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
#include <vector>

#include "base/memory/scoped_ptr.h"
#include "pagespeed/dom/resource_coordinate_finder.h"
#include "pagespeed/testing/pagespeed_test.h"

using pagespeed::dom::FindOnAndOffscreenImageResources;
using pagespeed_testing::FakeDomDocument;
using pagespeed_testing::FakeDomElement;
using pagespeed_testing::PagespeedTest;

namespace {

class FindOnAndOffscreenImageResourcesTest : public PagespeedTest {
 protected:
  static const char* kRootUrl;
  static const char* kIframeUrl;
  static const char* kImg1Url;
  static const char* kImg2Url;
  static const char* kAboveTheFoldUrl;

  virtual void DoSetUp() {
    SetViewportWidthAndHeight(1024, 768);
    NewPrimaryResource(kRootUrl);
    CreateHtmlHeadBodyElements();
  }

  void AddImage(const char* url, FakeDomElement* parent,
                int x, int y, int width, int height) {
    FakeDomElement* img = NULL;
    NewPngResource(url, parent, &img);
    img->SetCoordinates(x, y);
    img->SetActualWidthAndHeight(width, height);
  }

  void AddImage(const char* url, int x, int y, int width, int height) {
    AddImage(url, body(), x, y, width, height);
  }

  void AddVisibleImage() {
    AddImage(kAboveTheFoldUrl, 5, 5, 10, 10);
  }

  void Run() {
    Freeze();
    ASSERT_TRUE(FindOnAndOffscreenImageResources(
        *pagespeed_input(), &onscreen_resources_, &offscreen_resources_));
  }

  const pagespeed::ResourceVector& onscreen_resources() {
    return onscreen_resources_;
  }

  const pagespeed::ResourceVector& offscreen_resources() {
    return offscreen_resources_;
  }

 private:
  pagespeed::ResourceVector onscreen_resources_;
  pagespeed::ResourceVector offscreen_resources_;
};

const char* FindOnAndOffscreenImageResourcesTest::kRootUrl = "http://test.com/";
const char* FindOnAndOffscreenImageResourcesTest::kIframeUrl =
    "http://test.com/frame.html";
const char* FindOnAndOffscreenImageResourcesTest::kImg1Url =
    "http://test.com/a.png";
const char* FindOnAndOffscreenImageResourcesTest::kImg2Url =
    "http://test.com/b.png";
const char* FindOnAndOffscreenImageResourcesTest::kAboveTheFoldUrl =
    "http://test.com/atf.png";

TEST_F(FindOnAndOffscreenImageResourcesTest, EmptyDom) {
  Run();
  ASSERT_TRUE(onscreen_resources().empty());
  ASSERT_TRUE(offscreen_resources().empty());
}

TEST_F(FindOnAndOffscreenImageResourcesTest, ImageMissingDimensions) {
  FakeDomElement* img1 = NULL;
  NewPngResource(kImg1Url, body(), &img1);
  Run();
  ASSERT_TRUE(onscreen_resources().empty());
  ASSERT_TRUE(offscreen_resources().empty());
}

TEST_F(FindOnAndOffscreenImageResourcesTest, ImageAboveTheFold) {
  AddImage(kImg1Url, 0, 0, 10, 10);
  Run();
  ASSERT_EQ(1U, onscreen_resources().size());
  ASSERT_TRUE(offscreen_resources().empty());

  const pagespeed::Resource& resource = **onscreen_resources().begin();
  ASSERT_EQ(kImg1Url, resource.GetRequestUrl());
}

TEST_F(FindOnAndOffscreenImageResourcesTest, ImageAboveTheFoldNoWidth) {
  AddImage(kImg1Url, 0, 0, 0, 10);
  Run();
  ASSERT_EQ(1U, offscreen_resources().size());
  ASSERT_TRUE(onscreen_resources().empty());

  const pagespeed::Resource& resource = **offscreen_resources().begin();
  ASSERT_EQ(kImg1Url, resource.GetRequestUrl());
}

TEST_F(FindOnAndOffscreenImageResourcesTest, ImageBelowTheFold) {
  AddImage(kImg1Url, 0, 768, 10, 10);
  Run();

  ASSERT_EQ(1U, offscreen_resources().size());
  ASSERT_TRUE(onscreen_resources().empty());

  const pagespeed::Resource& resource = **offscreen_resources().begin();
  ASSERT_EQ(kImg1Url, resource.GetRequestUrl());
}

TEST_F(FindOnAndOffscreenImageResourcesTest, TwoImagesBelowTheFold) {
  AddImage(kImg1Url, 0, 768, 10, 10);
  AddImage(kImg2Url, 0, 1000, 10, 10);
  Run();

  ASSERT_EQ(2U, offscreen_resources().size());
  ASSERT_TRUE(onscreen_resources().empty());

  const pagespeed::Resource& resource1 = **offscreen_resources().begin();
  const pagespeed::Resource& resource2 = **(++offscreen_resources().begin());
  ASSERT_EQ(kImg1Url, resource1.GetRequestUrl());
  ASSERT_EQ(kImg2Url, resource2.GetRequestUrl());
}

TEST_F(FindOnAndOffscreenImageResourcesTest, ImageOverlappingTheFold) {
  AddImage(kImg1Url, 0, 760, 10, 10);
  Run();

  ASSERT_EQ(1U, onscreen_resources().size());
  ASSERT_TRUE(offscreen_resources().empty());

  const pagespeed::Resource& resource = **onscreen_resources().begin();
  ASSERT_EQ(kImg1Url, resource.GetRequestUrl());
}

TEST_F(FindOnAndOffscreenImageResourcesTest, SameImageAboveAndBelowTheFold) {
  AddImage(kImg1Url, 0, 768, 10, 10);

  FakeDomElement* img2 = FakeDomElement::NewImg(body(), kImg1Url);
  img2->SetCoordinates(0, 0);
  img2->SetActualWidthAndHeight(10, 10);

  Run();

  // When an image appears both above and below the fold, we consider
  // it to be above the fold.
  ASSERT_EQ(1U, onscreen_resources().size());
  ASSERT_TRUE(offscreen_resources().empty());

  const pagespeed::Resource& resource = **onscreen_resources().begin();
  ASSERT_EQ(kImg1Url, resource.GetRequestUrl());
}

TEST_F(FindOnAndOffscreenImageResourcesTest, OneImageVisibleOneNotVisible) {
  AddImage(kImg1Url, 1024, 100, 10, 10);
  AddImage(kImg2Url, 100, 100, 10, 10);
  Run();

  ASSERT_EQ(1U, onscreen_resources().size());
  ASSERT_EQ(1U, offscreen_resources().size());

  const pagespeed::Resource& offscreen_resource =
      **offscreen_resources().begin();
  ASSERT_EQ(kImg1Url, offscreen_resource.GetRequestUrl());

  const pagespeed::Resource& onscreen_resource = **onscreen_resources().begin();
  ASSERT_EQ(kImg2Url, onscreen_resource.GetRequestUrl());
}

TEST_F(FindOnAndOffscreenImageResourcesTest, RedirectedImage) {
  FakeDomElement* img1 = NULL;
  NewRedirectedPngResource(kImg1Url, kImg2Url, body(), &img1);
  img1->SetCoordinates(1024, 100);
  img1->SetActualWidthAndHeight(10, 10);
  Run();

  ASSERT_EQ(1U, offscreen_resources().size());
  ASSERT_TRUE(onscreen_resources().empty());

  const pagespeed::Resource& offscreen_resource =
      **offscreen_resources().begin();
  ASSERT_EQ(kImg2Url, offscreen_resource.GetRequestUrl());
}

TEST_F(FindOnAndOffscreenImageResourcesTest, ImageInIframe) {
  FakeDomElement* iframe = FakeDomElement::NewIframe(body());
  iframe->SetCoordinates(200, 200);
  iframe->SetActualWidthAndHeight(200, 200);
  FakeDomDocument* iframe_doc = NULL;
  NewDocumentResource(kIframeUrl, iframe, &iframe_doc);
  FakeDomElement* html = FakeDomElement::NewRoot(iframe_doc, "html");

  // 200, 200 + 0, 0 = 200, 200, which is above the fold.
  AddImage(kImg1Url, html, 0, 0, 10, 10);

  // 200, 200 + 0, 700 = 200, 900, which is below the fold.
  AddImage(kImg2Url, html, 0, 700, 10, 10);

  Run();

  ASSERT_EQ(1U, onscreen_resources().size());
  ASSERT_EQ(1U, offscreen_resources().size());

  const pagespeed::Resource& offscreen_resource =
      **offscreen_resources().begin();
  ASSERT_EQ(kImg2Url, offscreen_resource.GetRequestUrl());

  const pagespeed::Resource& onscreen_resource = **onscreen_resources().begin();
  ASSERT_EQ(kImg1Url, onscreen_resource.GetRequestUrl());
}

}  // namespace
