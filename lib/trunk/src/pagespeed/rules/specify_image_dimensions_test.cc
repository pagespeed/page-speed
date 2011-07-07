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

#include "base/scoped_ptr.h"
#include "base/stl_util-inl.h"  // for STLDeleteContainerPointers
#include "pagespeed/core/dom.h"
#include "pagespeed/core/pagespeed_input.h"
#include "pagespeed/core/resource.h"
#include "pagespeed/core/result_provider.h"
#include "pagespeed/proto/pagespeed_output.pb.h"
#include "pagespeed/rules/specify_image_dimensions.h"
#include "pagespeed/testing/pagespeed_test.h"

namespace {

using pagespeed::rules::SpecifyImageDimensions;
using pagespeed::Result;
using pagespeed::Results;
using pagespeed::ResultProvider;
using pagespeed_testing::FakeDomDocument;
using pagespeed_testing::FakeDomElement;
using pagespeed_testing::FakeImageAttributesFactory;
using pagespeed_testing::PagespeedRuleTest;

class SpecifyImageDimensionsTest
    : public PagespeedRuleTest<SpecifyImageDimensions> {
 protected:
  static const char* kRootUrl;
  static const char* kImgUrl;
  static const char* kRedirectUrl;

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

const char* SpecifyImageDimensionsTest::kRootUrl = "http://test.com/";
const char* SpecifyImageDimensionsTest::kImgUrl = "http://test.com/image.png";
const char* SpecifyImageDimensionsTest::kRedirectUrl =
    "http://test.com/redirect/image.png";

TEST_F(SpecifyImageDimensionsTest, EmptyDom) {
  CheckNoViolations();
}

TEST_F(SpecifyImageDimensionsTest, DimensionsSpecified) {
  FakeDomElement* img_element;
  NewPngResource(kImgUrl, body(), &img_element);
  img_element->AddAttribute("width", "23");
  img_element->AddAttribute("height", "42");
  CheckNoViolations();
}

TEST_F(SpecifyImageDimensionsTest, NoHeight) {
  FakeDomElement* img_element;
  NewPngResource(kImgUrl, body(), &img_element);
  img_element->AddAttribute("width", "23");
  CheckOneUrlViolation(kImgUrl);
}

TEST_F(SpecifyImageDimensionsTest, NoWidth) {
  FakeDomElement* img_element;
  NewPngResource(kImgUrl, body(), &img_element);
  img_element->AddAttribute("height", "42");
  CheckOneUrlViolation(kImgUrl);
}

TEST_F(SpecifyImageDimensionsTest, NoDimensions) {
  NewPngResource(kImgUrl, body());
  CheckOneUrlViolation(kImgUrl);
}

// See http://code.google.com/p/page-speed/issues/detail?id=459
TEST_F(SpecifyImageDimensionsTest, DataUrl) {
  // Create a data URL image tag, with no additional dimensions attributes.
  const std::string url =
      "data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAUAAAAFCAYAAACNbyblAAA"
      "AHElEQVQI12P4//8/w38GIAXDIBKE0DHxgljNBAAO9TXL0Y4OHwAAAABJRU5ErkJggg==";
  FakeDomElement::NewImg(body(), url);
  CheckNoViolations();
}

// Same test as above, only no resource URL specified. Now we expect
// no violation since a resource URL is required in order to trigger a
// violation.
TEST_F(SpecifyImageDimensionsTest, NoViolationMissingResourceUrl) {
  FakeDomElement* img_element;
  NewPngResource(kImgUrl, body(), &img_element);
  img_element->RemoveAttribute("src");
  CheckNoViolations();
}

TEST_F(SpecifyImageDimensionsTest, NoDimensionsInIFrame) {
  FakeDomElement* iframe = FakeDomElement::NewIframe(body());
  FakeDomDocument* iframe_doc;
  NewDocumentResource("http://test.com/frame/i.html", iframe, &iframe_doc);
  FakeDomElement* html2 = FakeDomElement::NewRoot(iframe_doc, "html");
  FakeDomElement* img_element;
  NewPngResource("http://test.com/frame/image.png", html2, &img_element);

  // Make the src attribute relative.
  img_element->AddAttribute("src", "image.png");

  CheckOneUrlViolation("http://test.com/frame/image.png");
}

TEST_F(SpecifyImageDimensionsTest, MultipleViolations) {
  NewPngResource(kImgUrl, body());
  FakeDomElement* img_element2;
  NewPngResource("http://test.com/imageB.png", body(), &img_element2);

  // Make the src attribute relative.
  img_element2->AddAttribute("src", "imageB.png");
  CheckTwoUrlViolations(kImgUrl, "http://test.com/imageB.png");
}

TEST_F(SpecifyImageDimensionsTest, RedirectTest) {
  std::string expected =
      "The following image(s) are missing width and/or height attributes.\n"
      "  http://test.com/redirect/image.png (Dimensions: 42 x 23)\n";

  pagespeed::Resource* resource =
      NewRedirectedPngResource(kRedirectUrl, kImgUrl, body());
  FakeImageAttributesFactory::ResourceSizeMap size_map;
  size_map[resource] = std::make_pair(42,23);
  AddFakeImageAttributesFactory(size_map);
  CheckFormattedOutput(expected);
}

TEST_F(SpecifyImageDimensionsTest, FormatTest) {
  std::string expected =
      "The following image(s) are missing width and/or height attributes.\n"
      "  http://test.com/image.png (Dimensions: 42 x 23)\n";

  pagespeed::Resource* resource = NewPngResource(kImgUrl, body());
  FakeImageAttributesFactory::ResourceSizeMap size_map;
  size_map[resource] = std::make_pair(42,23);
  AddFakeImageAttributesFactory(size_map);
  CheckFormattedOutput(expected);
}

TEST_F(SpecifyImageDimensionsTest, FormatNoImageDimensionsTest) {
  std::string expected =
      "The following image(s) are missing width and/or height attributes.\n"
      "  http://test.com/image.png\n";

  NewPngResource(kImgUrl, body());
  CheckFormattedOutput(expected);
}

TEST_F(SpecifyImageDimensionsTest, FormatNoOutputTest) {
  CheckFormattedOutput("");
}

}  // namespace
