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
#include "pagespeed/core/image_attributes.h"
#include "pagespeed/core/pagespeed_input.h"
#include "pagespeed/core/resource.h"
#include "pagespeed/core/result_provider.h"
#include "pagespeed/formatters/text_formatter.h"
#include "pagespeed/proto/pagespeed_output.pb.h"
#include "pagespeed/rules/specify_image_dimensions.h"
#include "pagespeed/testing/pagespeed_test.h"

using pagespeed::rules::SpecifyImageDimensions;
using pagespeed::PagespeedInput;
using pagespeed::Result;
using pagespeed::Results;
using pagespeed::ResultProvider;

namespace {

class MockImageAttributesFactory
    : public pagespeed::ImageAttributesFactory {
 public:
  virtual pagespeed::ImageAttributes* NewImageAttributes(
      const pagespeed::Resource* resource) const {
    return new pagespeed::ConcreteImageAttributes(42, 23);
  }
};

class MockDocument : public pagespeed::DomDocument {
 public:
  explicit MockDocument(const std::string& document_url)
      : document_url_(document_url), is_clone_(false) {}
  virtual ~MockDocument() {
    if (!is_clone_) {
      STLDeleteContainerPointers(elements_.begin(), elements_.end());
    }
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

  MockDocument* Clone() {
    MockDocument* doc = new MockDocument(GetDocumentUrl());
    doc->elements_ = elements_;
    doc->is_clone_ = true;
    return doc;
  }

 private:
  const std::string document_url_;
  std::vector<pagespeed::DomElement*> elements_;
  bool is_clone_;

  DISALLOW_COPY_AND_ASSIGN(MockDocument);
};

class MockElement : public pagespeed::DomElement {
 public:
  // Creates and returns a MockElement (which takes ownership of content).
  static MockElement* New(
      MockDocument* content,
      const std::string& tagname,
      const std::map<std::string, std::string>& attributes) {
    return new MockElement(content, tagname, attributes);
  }

  virtual pagespeed::DomDocument* GetContentDocument() const {
    return content_->Clone();
  }

  virtual std::string GetTagName() const {
    return tagname_;
  }

  virtual bool GetAttributeByName(const std::string& name,
                                  std::string* attr_value) const {
    std::map<std::string, std::string>::const_iterator entry =
        attributes_.find(name);
    if (entry != attributes_.end()) {
      *attr_value = entry->second;
      return true;
    } else {
      return false;
    }
  }

  virtual Status HasHeightSpecified(bool *out) const {
    *out = attributes_.find("height") != attributes_.end();
    return SUCCESS;
  }

  virtual Status HasWidthSpecified(bool *out) const {
    *out = attributes_.find("width") != attributes_.end();
    return SUCCESS;
  }

 private:
  // MockElement takes ownership of content.
  MockElement(MockDocument* content,
              const std::string& tagname,
              const std::map<std::string, std::string>& attributes)
      : content_(content),
        tagname_(tagname),
        attributes_(attributes) {
  }

  mutable scoped_ptr<MockDocument> content_;
  std::string tagname_;
  std::map<std::string, std::string> attributes_;

  DISALLOW_COPY_AND_ASSIGN(MockElement);
};

class SpecifyImageDimensionsTest : public ::pagespeed_testing::PagespeedTest {
 protected:
  MockDocument* NewMockDocument(const std::string& url) {
    AddResource(url.c_str(), "text/html");
    return new MockDocument(url);
  }

  void AddImageAttributesFactory() {
    input_->AcquireImageAttributesFactory(new MockImageAttributesFactory());
  }

  void AddResource(const char* url, const char* content_type) {
    pagespeed::Resource* resource = new pagespeed::Resource;
    resource->SetRequestUrl(url);
    resource->SetRequestMethod("GET");
    resource->SetResponseStatusCode(200);
    resource->AddResponseHeader("Content-Type", content_type);
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
  void CheckExpectedViolations(MockDocument* document,
                               const std::vector<std::string>& expected) {
    input_->AcquireDomDocument(document);
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

TEST_F(SpecifyImageDimensionsTest, EmptyDom) {
  MockDocument* doc = NewMockDocument("http://test.com/");
  CheckNoViolations(doc);
}

TEST_F(SpecifyImageDimensionsTest, DimensionsSpecified) {
  MockDocument* doc = NewMockDocument("http://test.com/");

  std::map<std::string, std::string> attributes;
  attributes["width"] = "23";
  attributes["height"] = "42";
  attributes["src"] = "http://test.com/image.png";
  AddResource("http://test.com/image.png", "image/png");
  doc->AddElement(MockElement::New(NULL,
                                   "IMG",
                                   attributes));
  CheckNoViolations(doc);
}

TEST_F(SpecifyImageDimensionsTest, NoHeight) {
  MockDocument* doc = NewMockDocument("http://test.com/");

  std::map<std::string, std::string> attributes;
  attributes["width"] = "23";
  attributes["src"] = "http://test.com/image.png";
  AddResource("http://test.com/image.png", "image/png");
  doc->AddElement(MockElement::New(NULL,
                                   "IMG",
                                   attributes));
  CheckOneViolation(doc, "http://test.com/image.png");
}

TEST_F(SpecifyImageDimensionsTest, NoWidth) {
  MockDocument* doc = NewMockDocument("http://test.com/");

  std::map<std::string, std::string> attributes;
  attributes["height"] = "42";
  attributes["src"] = "http://test.com/image.png";
  AddResource("http://test.com/image.png", "image/png");
  doc->AddElement(MockElement::New(NULL,
                                   "IMG",
                                   attributes));
  CheckOneViolation(doc, "http://test.com/image.png");
}

TEST_F(SpecifyImageDimensionsTest, NoDimensions) {
  MockDocument* doc = NewMockDocument("http://test.com/");

  std::map<std::string, std::string> attributes;
  attributes["src"] = "http://test.com/image.png";
  AddResource("http://test.com/image.png", "image/png");
  doc->AddElement(MockElement::New(NULL,
                                   "IMG",
                                   attributes));
  CheckOneViolation(doc, "http://test.com/image.png");
}

// Same test as above, only no resource URL specified. Now we expect
// no violation since a resource URL is required in order to trigger a
// violation.
TEST_F(SpecifyImageDimensionsTest, NoViolationMissingResourceUrl) {
  MockDocument* doc = NewMockDocument("http://test.com/");

  std::map<std::string, std::string> attributes;
  doc->AddElement(MockElement::New(NULL,
                                   "IMG",
                                   attributes));
  CheckNoViolations(doc);
}

TEST_F(SpecifyImageDimensionsTest, NoDimensionsInIFrame) {
  MockDocument* iframe_doc = NewMockDocument("http://test.com/frame/i.html");

  std::map<std::string, std::string> attributes;
  attributes["src"] = "image.png";
  AddResource("http://test.com/frame/image.png", "image/png");
  iframe_doc->AddElement(
      MockElement::New(NULL,
                       "IMG",
                       attributes));

  MockDocument* doc = NewMockDocument("http://test.com/");
  doc->AddElement(
      MockElement::New(iframe_doc,
                       "IFRAME",
                       std::map<std::string, std::string>()));

  CheckOneViolation(doc, "http://test.com/frame/image.png");
}

TEST_F(SpecifyImageDimensionsTest, MultipleViolations) {
  MockDocument* doc = NewMockDocument("http://test.com/");

  std::map<std::string, std::string> attributesA;
  attributesA["src"] = "http://test.com/imageA.png";
  AddResource("http://test.com/imageA.png", "image/png");
  doc->AddElement(MockElement::New(NULL,
                                   "IMG",
                                   attributesA));

  std::map<std::string, std::string> attributesB;
  attributesB["src"] = "imageB.png";
  AddResource("http://test.com/imageB.png", "image/png");
  doc->AddElement(MockElement::New(NULL,
                                   "IMG",
                                   attributesB));
  CheckTwoViolations(doc,
                     "http://test.com/imageA.png",
                     "http://test.com/imageB.png");
}

TEST_F(SpecifyImageDimensionsTest, FormatTest) {
  std::string expected =
      "The following image(s) are missing width and/or height attributes.\n"
      "  http://test.com/image.png (Dimensions: 42 x 23)\n";

  AddImageAttributesFactory();
  MockDocument* doc = NewMockDocument("http://test.com/");
  std::map<std::string, std::string> attributes;
  attributes["src"] = "http://test.com/image.png";
  AddResource("http://test.com/image.png", "image/png");
  doc->AddElement(MockElement::New(NULL,
                                   "IMG",
                                   attributes));
  CheckFormattedOutput(doc, expected);
}

TEST_F(SpecifyImageDimensionsTest, FormatNoImageDimensionsTest) {
  std::string expected =
      "The following image(s) are missing width and/or height attributes.\n"
      "  http://test.com/image.png\n";

  MockDocument* doc = NewMockDocument("http://test.com/");
  std::map<std::string, std::string> attributes;
  attributes["src"] = "http://test.com/image.png";
  AddResource("http://test.com/image.png", "image/png");
  doc->AddElement(MockElement::New(NULL,
                                   "IMG",
                                   attributes));
  CheckFormattedOutput(doc, expected);
}

TEST_F(SpecifyImageDimensionsTest, FormatNoOutputTest) {
  MockDocument* doc = NewMockDocument("http://test.com/");
  CheckFormattedOutput(doc, "");
}

}  // namespace
