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
#include "pagespeed/core/image_attributes.h"
#include "pagespeed/core/pagespeed_input.h"
#include "pagespeed/core/resource.h"
#include "pagespeed/core/result_provider.h"
#include "pagespeed/formatters/text_formatter.h"
#include "pagespeed/proto/pagespeed_output.pb.h"
#include "pagespeed/rules/serve_scaled_images.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

class MockImageAttributesFactory : public pagespeed::ImageAttributesFactory {
 public:
  virtual pagespeed::ImageAttributes* NewImageAttributes(
      const pagespeed::Resource* resource) const {
    return new pagespeed::ConcreteImageAttributes(23, 42);
  }
};

class MockDocument : public pagespeed::DomDocument {
 public:
  explicit MockDocument(const std::string& document_url)
      : document_url_(document_url) {}
  virtual ~MockDocument() {
    STLDeleteContainerPointers(elements_.begin(), elements_.end());
  }

  virtual std::string GetDocumentUrl() const {
    return document_url_;
  }

  virtual void Traverse(pagespeed::DomElementVisitor* visitor) const {
    for (std::vector<pagespeed::DomElement*>::const_iterator
             iter = elements_.begin(),
             end = elements_.end();
         iter != end;
         ++iter) {
      visitor->Visit(**iter);
    }
  }

  void AddElement(pagespeed::DomElement* element) {
    elements_.push_back(element);
  }

 private:
  const std::string document_url_;
  std::vector<pagespeed::DomElement*> elements_;

  DISALLOW_COPY_AND_ASSIGN(MockDocument);
};

class MockImageElement : public pagespeed::DomElement {
 public:
  MockImageElement(const std::string& resource_url,
                   int client_width, int client_height)
      : resource_url_(resource_url),
        client_width_(client_width), client_height_(client_height) {}

  virtual pagespeed::DomDocument* GetContentDocument() const {
    return NULL;
  }

  virtual std::string GetTagName() const {
    return "IMG";
  }

  virtual bool GetAttributeByName(const std::string& name,
                                  std::string* attr_value) const {
    if (name == "src") {
      attr_value->assign(resource_url_);
      return true;
    } else {
      return false;
    }
  }

  virtual Status GetActualWidth(int* property_value) const {
    *property_value = client_width_;
    return SUCCESS;
  }

  virtual Status GetActualHeight(int* property_value) const {
    *property_value = client_height_;
    return SUCCESS;
  }

 private:
  std::string resource_url_;
  int client_width_, client_height_;

  DISALLOW_COPY_AND_ASSIGN(MockImageElement);
};

class MockIframeElement : public pagespeed::DomElement {
 public:
  // MockIframeElement takes ownership of content.
  explicit MockIframeElement(pagespeed::DomDocument* content)
      : content_(content) {}

  // Ownership is transferred to the caller. May be NULL.
  virtual pagespeed::DomDocument* GetContentDocument() const {
    return content_.release();
  }

  virtual std::string GetTagName() const {
    return "IFRAME";
  }

  virtual bool GetAttributeByName(const std::string& name,
                                  std::string* out) const {
    return false;
  }

 private:
  mutable scoped_ptr<pagespeed::DomDocument> content_;

  DISALLOW_COPY_AND_ASSIGN(MockIframeElement);
};

class ServeScaledImagesTest : public ::testing::Test {
 protected:

  virtual void SetUp() {
    input_.reset(new pagespeed::PagespeedInput);
    input_->AcquireImageAttributesFactory(new MockImageAttributesFactory());
  }

  virtual void TearDown() {
    input_.reset();
  }

  MockDocument* NewMockDocument (const std::string& url) {
    pagespeed::Resource* resource = new pagespeed::Resource;
    resource->SetRequestUrl(url);
    resource->SetRequestMethod("GET");
    resource->SetResponseStatusCode(200);
    resource->AddResponseHeader("Content-Type", "text/html");
    input_->AddResource(resource);
    return new MockDocument(url);
  }

  void AddPngResource(const std::string &url, const int size) {
    std::string body(size, 'x');
    pagespeed::Resource* resource = new pagespeed::Resource;
    resource->SetRequestUrl(url);
    resource->SetRequestMethod("GET");
    resource->SetRequestProtocol("HTTP");
    resource->SetResponseStatusCode(200);
    resource->SetResponseProtocol("HTTP/1.1");
    resource->AddResponseHeader("Content-Type", "image/png");
    resource->SetResponseBody(body);
    input_->AddResource(resource);
  }

  void CheckNoViolations(MockDocument* document) {
    CheckExpectedViolations(document, std::vector<std::string>());
  }

  void CheckOneViolation(MockDocument* document,
                         const std::string& violation_url) {
    std::vector<std::string> expected;
    expected.push_back(violation_url);
    CheckExpectedViolations(document, expected);
  }

  void CheckTwoViolations(MockDocument* document,
                          const std::string& violation_url1,
                          const std::string& violation_url2) {
    std::vector<std::string> expected;
    expected.push_back(violation_url1);
    expected.push_back(violation_url2);
    CheckExpectedViolations(document, expected);
  }

  void CheckFormattedOutput(MockDocument* document,
                            const std::string& expected_output) {
    input_->AcquireDomDocument(document);

    pagespeed::Results results;
    {
      // compute results
      pagespeed::rules::ServeScaledImages scaling_rule;
      pagespeed::ResultProvider provider(scaling_rule, &results);
      ASSERT_TRUE(scaling_rule.AppendResults(*input_, &provider));
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
  void CheckExpectedViolations(MockDocument* document,
                               const std::vector<std::string>& expected) {
    input_->AcquireDomDocument(document);

    pagespeed::rules::ServeScaledImages scaling_rule;

    pagespeed::Results results;
    pagespeed::ResultProvider provider(scaling_rule, &results);
    ASSERT_TRUE(scaling_rule.AppendResults(*input_, &provider));
    ASSERT_EQ(expected.size(), static_cast<size_t>(results.results_size()));

    for (size_t idx = 0; idx < expected.size(); ++idx) {
      const pagespeed::Result& result = results.results(idx);
      ASSERT_EQ(result.resource_urls_size(), 1);
      EXPECT_EQ(expected[idx], result.resource_urls(0));
    }
  }

  scoped_ptr<pagespeed::PagespeedInput> input_;
};

TEST_F(ServeScaledImagesTest, EmptyDom) {
  MockDocument* doc = NewMockDocument("http://test.com/");
  CheckNoViolations(doc);
}

TEST_F(ServeScaledImagesTest, NotResized) {
  MockDocument* doc = NewMockDocument("http://test.com/");
  AddPngResource("http://test.com/image.png", 50);
  doc->AddElement(new MockImageElement("http://test.com/image.png",
                                       23, 42));
  CheckNoViolations(doc);
}

TEST_F(ServeScaledImagesTest, ShrunkHeight) {
  MockDocument* doc = NewMockDocument("http://test.com/");
  AddPngResource("http://test.com/image.png", 50);
  doc->AddElement(new MockImageElement("http://test.com/image.png",
                                       23, 21));
  CheckOneViolation(doc, "http://test.com/image.png");
}

TEST_F(ServeScaledImagesTest, ShrunkWidth) {
  MockDocument* doc = NewMockDocument("http://test.com/");
  AddPngResource("http://test.com/image.png", 50);
  doc->AddElement(new MockImageElement("http://test.com/image.png",
                                       22, 42));
  CheckOneViolation(doc, "http://test.com/image.png");
}

TEST_F(ServeScaledImagesTest, ShrunkBoth) {
  MockDocument* doc = NewMockDocument("http://test.com/");
  AddPngResource("http://test.com/image.png", 50);
  doc->AddElement(new MockImageElement("http://test.com/image.png",
                                       22, 21));
  CheckOneViolation(doc, "http://test.com/image.png");
}

TEST_F(ServeScaledImagesTest, IncreasedBoth) {
  MockDocument* doc = NewMockDocument("http://test.com/");
  AddPngResource("http://test.com/image.png", 50);
  doc->AddElement(new MockImageElement("http://test.com/image.png",
                                       46, 84));
  CheckNoViolations(doc);
}

TEST_F(ServeScaledImagesTest, ShrunkInIFrame) {
  MockDocument* iframe_doc = NewMockDocument("http://test.com/frame/i.html");
  AddPngResource("http://test.com/frame/image.png", 50);
  iframe_doc->AddElement(new MockImageElement("image.png",
                                              22, 21));
  MockDocument* doc = NewMockDocument("http://test.com/");
  doc->AddElement(new MockIframeElement(iframe_doc));
  CheckOneViolation(doc, "http://test.com/frame/image.png");
}

TEST_F(ServeScaledImagesTest, MultipleViolations) {
  MockDocument* doc = NewMockDocument("http://test.com/");
  AddPngResource("http://test.com/imageA.png", 50);
  AddPngResource("http://test.com/imageB.png", 40);
  doc->AddElement(new MockImageElement("http://test.com/imageA.png",
                                       22, 21));
  doc->AddElement(new MockImageElement("imageB.png",
                                       5, 15));
  CheckTwoViolations(doc,
                     "http://test.com/imageA.png",
                     "http://test.com/imageB.png");
}

TEST_F(ServeScaledImagesTest, ShrunkTwice) {
  MockDocument* doc = NewMockDocument("http://test.com/");
  AddPngResource("http://test.com/image.png", 50);
  doc->AddElement(new MockImageElement("http://test.com/image.png",
                                       22, 21));
  doc->AddElement(new MockImageElement("http://test.com/image.png",
                                       5, 15));
  CheckOneViolation(doc, "http://test.com/image.png");
}

TEST_F(ServeScaledImagesTest, NotAlwaysShrunk) {
  MockDocument* doc = NewMockDocument("http://test.com/");
  AddPngResource("http://test.com/image.png", 50);
  doc->AddElement(new MockImageElement("http://test.com/image.png",
                                       23, 42));
  doc->AddElement(new MockImageElement("http://test.com/image.png",
                                       5, 15));
  CheckNoViolations(doc);
}

TEST_F(ServeScaledImagesTest, ShrunkAndIncreased) {
  MockDocument* doc = NewMockDocument("http://test.com/");
  AddPngResource("http://test.com/image.png", 50);
  doc->AddElement(new MockImageElement("http://test.com/image.png",
                                       46, 84));
  doc->AddElement(new MockImageElement("http://test.com/image.png",
                                       5, 15));
  CheckNoViolations(doc);
}

TEST_F(ServeScaledImagesTest, FormatTest) {
  std::string expected =
      "The following images are resized in HTML or CSS.  "
      "Serving scaled images could save 47B (94% reduction).\n"
      "  http://test.com/a.png is resized in HTML or CSS from 23x42 to 5x15.  "
      "Serving a scaled image could save 47B (94% reduction).\n";

  MockDocument* doc = NewMockDocument("http://test.com/");
  AddPngResource("http://test.com/a.png", 50);
  doc->AddElement(new MockImageElement("http://test.com/a.png", 5, 15));
  CheckFormattedOutput(doc, expected);
}

TEST_F(ServeScaledImagesTest, FormatNoOutputTest) {
  MockDocument* doc = NewMockDocument("http://test.com/");
  CheckFormattedOutput(doc, "");
}

}  // namespace
