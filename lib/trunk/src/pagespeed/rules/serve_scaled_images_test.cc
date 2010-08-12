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
#include <sstream>

#include "base/scoped_ptr.h"
#include "base/stl_util-inl.h"  // for STLDeleteContainerPointers
#include "pagespeed/core/dom.h"
#include "pagespeed/core/pagespeed_input.h"
#include "pagespeed/core/resource.h"
#include "pagespeed/core/result_provider.h"
#include "pagespeed/formatters/text_formatter.h"
#include "pagespeed/proto/pagespeed_output.pb.h"
#include "pagespeed/rules/serve_scaled_images.h"
#include "pagespeed/testing/pagespeed_test.h"
#include "pagespeed/testing/pagespeed_test.h"

namespace {

using pagespeed_testing::FakeDomDocument;
using pagespeed_testing::FakeDomElement;

class ServeScaledImagesTest : public ::pagespeed_testing::PagespeedTest {
 protected:
  static const char* kRootUrl;
  static const char* kImgUrl;
  static const int kImgSizeBytes;

  virtual void DoSetUp() {
    NewPrimaryResource(kRootUrl);
    CreateHtmlHeadBodyElements();
    AddFakeImageAttributesFactory();
  }

  FakeDomElement* CreatePngElement(const std::string& url, FakeDomElement* parent) {
    FakeDomElement* element;
    pagespeed::Resource* resource = NewPngResource(url, parent, &element);
    std::string body(kImgSizeBytes, 'x');
    resource->SetResponseBody(body);
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
    Freeze();

    pagespeed::Results results;
    {
      // compute results
      pagespeed::rules::ServeScaledImages scaling_rule;
      pagespeed::ResultProvider provider(scaling_rule, &results);
      ASSERT_TRUE(scaling_rule.AppendResults(*input(), &provider));
    }

    {
      // format results
      pagespeed::ResultVector result_vector;
      for (int ii = 0; ii < results.results_size(); ++ii) {
        result_vector.push_back(&results.results(ii));
      }

      std::stringstream output;
      pagespeed::formatters::TextFormatter formatter(&output);
      pagespeed::rules::ServeScaledImages scaling_rule;
      scaling_rule.FormatResults(result_vector, &formatter);
      EXPECT_STREQ(expected_output.c_str(), output.str().c_str());
    }
  }

 private:
  void CheckExpectedViolations(const std::vector<std::string>& expected) {
    Freeze();

    pagespeed::rules::ServeScaledImages scaling_rule;

    pagespeed::Results results;
    pagespeed::ResultProvider provider(scaling_rule, &results);
    ASSERT_TRUE(scaling_rule.AppendResults(*input(), &provider));
    ASSERT_EQ(expected.size(), static_cast<size_t>(results.results_size()));

    for (size_t idx = 0; idx < expected.size(); ++idx) {
      const pagespeed::Result& result = results.results(idx);
      ASSERT_EQ(result.resource_urls_size(), 1);
      EXPECT_EQ(expected[idx], result.resource_urls(0));
    }
  }
};

const char* ServeScaledImagesTest::kRootUrl = "http://test.com/";
const char* ServeScaledImagesTest::kImgUrl = "http://test.com/image.png";
const int ServeScaledImagesTest::kImgSizeBytes = 50;

TEST_F(ServeScaledImagesTest, EmptyDom) {
  CheckNoViolations();
}

TEST_F(ServeScaledImagesTest, NotResized) {
  FakeDomElement* element = CreatePngElement(kImgUrl, body());
  element->SetActualWidthAndHeight(42, 23);
  CheckNoViolations();
}

TEST_F(ServeScaledImagesTest, ShrunkHeight) {
  FakeDomElement* element = CreatePngElement(kImgUrl, body());
  element->SetActualWidthAndHeight(21, 23);
  CheckOneViolation(kImgUrl);
}

TEST_F(ServeScaledImagesTest, ShrunkWidth) {
  FakeDomElement* element = CreatePngElement(kImgUrl, body());
  element->SetActualWidthAndHeight(42, 22);
  CheckOneViolation(kImgUrl);
}

TEST_F(ServeScaledImagesTest, ShrunkBoth) {
  FakeDomElement* element = CreatePngElement(kImgUrl, body());
  element->SetActualWidthAndHeight(21, 22);
  CheckOneViolation(kImgUrl);
}

TEST_F(ServeScaledImagesTest, IncreasedBoth) {
  FakeDomElement* element = CreatePngElement(kImgUrl, body());
  element->SetActualWidthAndHeight(84, 46);
  CheckNoViolations();
}

TEST_F(ServeScaledImagesTest, ShrunkInIFrame) {
  FakeDomElement* iframe = FakeDomElement::NewIframe(body());
  FakeDomDocument* iframe_doc;
  NewDocumentResource("http://test.com/frame/i.html", iframe, &iframe_doc);
  FakeDomElement* html2 = FakeDomElement::NewRoot(iframe_doc, "html");
  FakeDomElement* element =
      CreatePngElement("http://test.com/frame/image.png", html2);
  element->SetActualWidthAndHeight(21, 22);
  CheckOneViolation("http://test.com/frame/image.png");
}

TEST_F(ServeScaledImagesTest, MultipleViolations) {
  FakeDomElement* elementA =
      CreatePngElement("http://test.com/imageA.png", body());
  elementA->SetActualWidthAndHeight(21, 22);
  FakeDomElement* elementB =
      CreatePngElement("http://test.com/imageB.png", body());
  elementB->SetActualWidthAndHeight(15, 5);
  CheckTwoViolations("http://test.com/imageA.png",
                     "http://test.com/imageB.png");
}

TEST_F(ServeScaledImagesTest, ShrunkTwice) {
  FakeDomElement* elementA = CreatePngElement(kImgUrl, body());
  elementA->SetActualWidthAndHeight(21, 22);
  FakeDomElement* elementB = FakeDomElement::NewImg(body(), kImgUrl);
  elementB->SetActualWidthAndHeight(15, 5);
  CheckOneViolation(kImgUrl);
}

TEST_F(ServeScaledImagesTest, NotAlwaysShrunk) {
  FakeDomElement* elementA = CreatePngElement(kImgUrl, body());
  elementA->SetActualWidthAndHeight(42, 23);
  FakeDomElement* elementB = FakeDomElement::NewImg(body(), kImgUrl);
  elementB->SetActualWidthAndHeight(15, 5);
  CheckNoViolations();
}

TEST_F(ServeScaledImagesTest, ShrunkAndIncreased) {
  FakeDomElement* elementA = CreatePngElement(kImgUrl, body());
  elementA->SetActualWidthAndHeight(84, 46);
  FakeDomElement* elementB = FakeDomElement::NewImg(body(), kImgUrl);
  elementB->SetActualWidthAndHeight(15, 5);
  CheckNoViolations();
}

TEST_F(ServeScaledImagesTest, FormatTest) {
  std::string expected =
      "The following images are resized in HTML or CSS.  "
      "Serving scaled images could save 47B (94% reduction).\n"
      "  http://test.com/image.png is resized in HTML or CSS from 42x23 to 15x5.  "
      "Serving a scaled image could save 47B (94% reduction).\n";

  FakeDomElement* element = CreatePngElement(kImgUrl, body());
  element->SetActualWidthAndHeight(15, 5);
  CheckFormattedOutput(expected);
}

TEST_F(ServeScaledImagesTest, FormatNoOutputTest) {
  CheckFormattedOutput("");
}

}  // namespace
