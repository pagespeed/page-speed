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

#include "pagespeed/rules/serve_scaled_images.h"
#include "pagespeed/testing/pagespeed_test.h"

namespace {

using pagespeed::rules::ServeScaledImages;
using pagespeed_testing::FakeDomDocument;
using pagespeed_testing::FakeDomElement;
using pagespeed_testing::FakeImageAttributesFactory;

class ServeScaledImagesTest
    : public ::pagespeed_testing::PagespeedRuleTest<ServeScaledImages> {
 protected:
  static const char* kRootUrl;
  static const char* kImgUrl;
  static const char* kRedirectUrl;
  static const int kImgSizeBytes;

  virtual void DoSetUp() {
    NewPrimaryResource(kRootUrl);
    CreateHtmlHeadBodyElements();
  }

  FakeDomElement* CreatePngElement(
      const std::string& url, FakeDomElement* parent,
      pagespeed::Resource** resource) {
    FakeDomElement* element;
    *resource = NewPngResource(url, parent, &element);
    std::string body(kImgSizeBytes, 'x');
    (*resource)->SetResponseBody(body);
    return element;
  }

  FakeDomElement* CreateRedirectedPngElement(
      const std::string& url1, const std::string& url2,
      FakeDomElement* parent, pagespeed::Resource** resource) {
    FakeDomElement* element;
    *resource = NewRedirectedPngResource(url1, url2, parent, &element);
    std::string body(kImgSizeBytes, 'x');
    (*resource)->SetResponseBody(body);
    return element;
  }

  void CheckNoViolations() {
    CheckExpectedViolations(std::vector<std::string>());
  }

  void CheckOneViolation(const std::string& violation_url) {
    std::vector<std::string> expected;
    expected.push_back(violation_url);
    CheckExpectedViolations(expected);
  }

  void CheckTwoViolations(const std::string& violation_url1,
                          const std::string& violation_url2) {
    std::vector<std::string> expected;
    expected.push_back(violation_url1);
    expected.push_back(violation_url2);
    CheckExpectedViolations(expected);
  }

  void CheckFormattedOutput(const std::string& expected_output) {
    ASSERT_TRUE(AppendResults());
    EXPECT_EQ(expected_output, FormatResults());
  }

 private:
  void CheckExpectedViolations(const std::vector<std::string>& expected) {
    ASSERT_TRUE(AppendResults());
    ASSERT_EQ(expected.size(), static_cast<size_t>(num_results()));

    for (size_t idx = 0; idx < expected.size(); ++idx) {
      ASSERT_EQ(result(idx).resource_urls_size(), 1);
      EXPECT_EQ(expected[idx], result(idx).resource_urls(0));
    }
  }
};

const char* ServeScaledImagesTest::kRootUrl = "http://test.com/";
const char* ServeScaledImagesTest::kImgUrl = "http://test.com/image.png";
const char* ServeScaledImagesTest::kRedirectUrl =
    "http://test.com/redirect/image.png";
const int ServeScaledImagesTest::kImgSizeBytes = 50;

TEST_F(ServeScaledImagesTest, EmptyDom) {
  Freeze();
  CheckNoViolations();
}

TEST_F(ServeScaledImagesTest, NotResized) {
  pagespeed::Resource* resource;
  FakeDomElement* element = CreatePngElement(kImgUrl, body(), &resource);
  element->SetActualWidthAndHeight(42, 23);
  FakeImageAttributesFactory::ResourceSizeMap size_map;
  size_map[resource] = std::make_pair(42,23);
  AddFakeImageAttributesFactory(size_map);
  Freeze();
  CheckNoViolations();
}

TEST_F(ServeScaledImagesTest, ShrunkHeight) {
  pagespeed::Resource* resource;
  FakeDomElement* element = CreatePngElement(kImgUrl, body(), &resource);
  element->SetActualWidthAndHeight(21, 23);
  FakeImageAttributesFactory::ResourceSizeMap size_map;
  size_map[resource] = std::make_pair(42,23);
  AddFakeImageAttributesFactory(size_map);
  Freeze();
  CheckOneViolation(kImgUrl);
}

TEST_F(ServeScaledImagesTest, ShrunkWidth) {
  pagespeed::Resource* resource;
  FakeDomElement* element = CreatePngElement(kImgUrl, body(), &resource);
  element->SetActualWidthAndHeight(42, 22);
  FakeImageAttributesFactory::ResourceSizeMap size_map;
  size_map[resource] = std::make_pair(42,23);
  AddFakeImageAttributesFactory(size_map);
  Freeze();
  CheckOneViolation(kImgUrl);
}

TEST_F(ServeScaledImagesTest, ShrunkBoth) {
  pagespeed::Resource* resource;
  FakeDomElement* element = CreatePngElement(kImgUrl, body(), &resource);
  element->SetActualWidthAndHeight(21, 22);
  FakeImageAttributesFactory::ResourceSizeMap size_map;
  size_map[resource] = std::make_pair(42,23);
  AddFakeImageAttributesFactory(size_map);
  Freeze();
  CheckOneViolation(kImgUrl);
}

TEST_F(ServeScaledImagesTest, IncreasedBoth) {
  pagespeed::Resource* resource;
  FakeDomElement* element = CreatePngElement(kImgUrl, body(), &resource);
  element->SetActualWidthAndHeight(84, 46);
  FakeImageAttributesFactory::ResourceSizeMap size_map;
  size_map[resource] = std::make_pair(42,23);
  AddFakeImageAttributesFactory(size_map);
  Freeze();
  CheckNoViolations();
}

TEST_F(ServeScaledImagesTest, ShrunkInIFrame) {
  FakeDomElement* iframe = FakeDomElement::NewIframe(body());
  FakeDomDocument* iframe_doc;
  NewDocumentResource("http://test.com/frame/i.html", iframe, &iframe_doc);
  FakeDomElement* html2 = FakeDomElement::NewRoot(iframe_doc, "html");
  pagespeed::Resource* resource;
  FakeDomElement* element =
      CreatePngElement("http://test.com/frame/image.png", html2, &resource);
  element->SetActualWidthAndHeight(21, 22);
  FakeImageAttributesFactory::ResourceSizeMap size_map;
  size_map[resource] = std::make_pair(42,23);
  AddFakeImageAttributesFactory(size_map);
  Freeze();
  CheckOneViolation("http://test.com/frame/image.png");
}

TEST_F(ServeScaledImagesTest, MultipleViolations) {
  pagespeed::Resource* resourceA;
  FakeDomElement* elementA =
      CreatePngElement("http://test.com/imageA.png", body(), &resourceA);
  elementA->SetActualWidthAndHeight(21, 22);
  pagespeed::Resource* resourceB;
  FakeDomElement* elementB =
      CreatePngElement("http://test.com/imageB.png", body(), &resourceB);
  elementB->SetActualWidthAndHeight(15, 5);

  FakeImageAttributesFactory::ResourceSizeMap size_map;
  size_map[resourceA] = std::make_pair(42,23);
  size_map[resourceB] = std::make_pair(42,23);
  AddFakeImageAttributesFactory(size_map);
  Freeze();
  CheckTwoViolations("http://test.com/imageA.png",
                     "http://test.com/imageB.png");
}

TEST_F(ServeScaledImagesTest, ShrunkTwice) {
  pagespeed::Resource* resourceA;
  FakeDomElement* elementA = CreatePngElement(kImgUrl, body(), &resourceA);
  elementA->SetActualWidthAndHeight(21, 22);
  FakeDomElement* elementB = FakeDomElement::NewImg(body(), kImgUrl);
  elementB->SetActualWidthAndHeight(15, 5);
  FakeImageAttributesFactory::ResourceSizeMap size_map;
  size_map[resourceA] = std::make_pair(42,23);
  AddFakeImageAttributesFactory(size_map);
  Freeze();
  CheckOneViolation(kImgUrl);
}

TEST_F(ServeScaledImagesTest, NotAlwaysShrunk) {
  pagespeed::Resource* resourceA;
  FakeDomElement* elementA = CreatePngElement(kImgUrl, body(), &resourceA);
  elementA->SetActualWidthAndHeight(42, 23);
  FakeDomElement* elementB = FakeDomElement::NewImg(body(), kImgUrl);
  elementB->SetActualWidthAndHeight(15, 5);
  FakeImageAttributesFactory::ResourceSizeMap size_map;
  size_map[resourceA] = std::make_pair(42,23);
  AddFakeImageAttributesFactory(size_map);
  Freeze();
  CheckNoViolations();
}

TEST_F(ServeScaledImagesTest, ShrunkAndIncreased) {
  pagespeed::Resource* resourceA;
  FakeDomElement* elementA = CreatePngElement(kImgUrl, body(), &resourceA);
  elementA->SetActualWidthAndHeight(84, 46);
  FakeDomElement* elementB = FakeDomElement::NewImg(body(), kImgUrl);
  elementB->SetActualWidthAndHeight(15, 5);
  FakeImageAttributesFactory::ResourceSizeMap size_map;
  size_map[resourceA] = std::make_pair(42,23);
  AddFakeImageAttributesFactory(size_map);
  Freeze();
  CheckNoViolations();
}

TEST_F(ServeScaledImagesTest, RedirectTest) {
  std::string expected =
      "The following images are resized in HTML or CSS. "
      "Serving scaled images could save 47B (94% reduction).\n"
      "  http://test.com/redirect/image.png is resized in HTML or CSS from "
      "42x23 to 15x5. "
      "Serving a scaled image could save 47B (94% reduction).\n";

  pagespeed::Resource* resource;
  FakeDomElement* element =
      CreateRedirectedPngElement(kRedirectUrl, kImgUrl, body(), &resource);
  element->SetActualWidthAndHeight(15, 5);
  FakeImageAttributesFactory::ResourceSizeMap size_map;
  size_map[resource] = std::make_pair(42,23);
  AddFakeImageAttributesFactory(size_map);
  Freeze();
  CheckFormattedOutput(expected);
}

TEST_F(ServeScaledImagesTest, FormatTest) {
  std::string expected =
      "The following images are resized in HTML or CSS. "
      "Serving scaled images could save 47B (94% reduction).\n"
      "  http://test.com/image.png is resized in HTML or CSS from "
      "42x23 to 15x5. "
      "Serving a scaled image could save 47B (94% reduction).\n";

  pagespeed::Resource* resource;
  FakeDomElement* element = CreatePngElement(kImgUrl, body(), &resource);
  element->SetActualWidthAndHeight(15, 5);
  FakeImageAttributesFactory::ResourceSizeMap size_map;
  size_map[resource] = std::make_pair(42,23);
  AddFakeImageAttributesFactory(size_map);
  Freeze();
  CheckFormattedOutput(expected);
}

TEST_F(ServeScaledImagesTest, FormatNoOutputTest) {
  Freeze();
  CheckFormattedOutput("");
}

}  // namespace
