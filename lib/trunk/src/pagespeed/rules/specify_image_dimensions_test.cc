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
#include "pagespeed/formatters/text_formatter.h"
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

class SpecifyImageDimensionsTest : public ::pagespeed_testing::PagespeedTest {
 protected:
  static const char* kRootUrl;
  static const char* kImgUrl;

  virtual void DoSetUp() {
    NewPrimaryResource(kRootUrl);
    CreateHtmlHeadBodyElements();
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
      SpecifyImageDimensions dimensions_rule;
      ResultProvider provider(dimensions_rule, &results);
      ASSERT_TRUE(dimensions_rule.AppendResults(*input(), &provider));
    }

    {
      // format results
      pagespeed::ResultVector result_vector;
      for (int ii = 0; ii < results.results_size(); ++ii) {
        result_vector.push_back(&results.results(ii));
      }

      std::stringstream output;
      pagespeed::formatters::TextFormatter formatter(&output);
      SpecifyImageDimensions dimensions_rule;
      dimensions_rule.FormatResults(result_vector, &formatter);
      EXPECT_STREQ(expected_output.c_str(), output.str().c_str());
    }
  }

 private:
  void CheckExpectedViolations(const std::vector<std::string>& expected) {
    Freeze();

    SpecifyImageDimensions dimensions_rule;

    Results results;
    ResultProvider provider(dimensions_rule, &results);
    ASSERT_TRUE(dimensions_rule.AppendResults(*input(), &provider));
    ASSERT_EQ(static_cast<size_t>(results.results_size()), expected.size());

    for (size_t idx = 0; idx < expected.size(); ++idx) {
      const Result& result = results.results(idx);
      ASSERT_EQ(result.resource_urls_size(), 1);
      EXPECT_EQ(expected[idx], result.resource_urls(0));
    }
  }
};

const char* SpecifyImageDimensionsTest::kRootUrl = "http://test.com/";
const char* SpecifyImageDimensionsTest::kImgUrl = "http://test.com/image.png";

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
  CheckOneViolation(kImgUrl);
}

TEST_F(SpecifyImageDimensionsTest, NoWidth) {
  FakeDomElement* img_element;
  NewPngResource(kImgUrl, body(), &img_element);
  img_element->AddAttribute("height", "42");
  CheckOneViolation(kImgUrl);
}

TEST_F(SpecifyImageDimensionsTest, NoDimensions) {
  NewPngResource(kImgUrl, body());
  CheckOneViolation(kImgUrl);
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

  CheckOneViolation("http://test.com/frame/image.png");
}

TEST_F(SpecifyImageDimensionsTest, MultipleViolations) {
  NewPngResource(kImgUrl, body());
  FakeDomElement* img_element2;
  NewPngResource("http://test.com/imageB.png", body(), &img_element2);

  // Make the src attribute relative.
  img_element2->AddAttribute("src", "imageB.png");
  CheckTwoViolations(kImgUrl, "http://test.com/imageB.png");
}

TEST_F(SpecifyImageDimensionsTest, FormatTest) {
  std::string expected =
      "The following image(s) are missing width and/or height attributes.\n"
      "  http://test.com/image.png (Dimensions: 42 x 23)\n";

  AddFakeImageAttributesFactory();
  NewPngResource(kImgUrl, body());
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
