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
#include "pagespeed/rules/put_css_in_the_document_head.h"
#include "pagespeed/testing/fake_dom.h"
#include "pagespeed/testing/pagespeed_test.h"

using pagespeed::DomDocument;
using pagespeed::StylesInBodyDetails;
using pagespeed_testing::FakeDomDocument;
using pagespeed_testing::FakeDomElement;

namespace {

class PutCssInTheDocumentHeadTest : public ::pagespeed_testing::PagespeedTest {
 protected:
  static const char* kRootUrl;

  virtual void DoSetUp() {
    document_.reset(NewFakeDomDocument(kRootUrl));
    html_ = FakeDomElement::NewRoot(document_.get(), "HTML");
    head_ = FakeDomElement::New(html_, "HEAD");
    body_ = FakeDomElement::New(html_, "BODY");
  }

  FakeDomDocument* NewFakeDomDocument(const std::string& url) {
    pagespeed::Resource* resource = new pagespeed::Resource;
    resource->SetRequestUrl(url);
    resource->SetRequestMethod("GET");
    resource->SetResponseStatusCode(200);
    resource->AddResponseHeader("Content-Type", "text/html");
    input_->AddResource(resource);
    return FakeDomDocument::NewRoot(url);
  }

  FakeDomDocument* NewFakeDomDocument(FakeDomElement* iframe,
                                      const std::string& url) {
    pagespeed::Resource* resource = new pagespeed::Resource;
    resource->SetRequestUrl(url);
    resource->SetRequestMethod("GET");
    resource->SetResponseStatusCode(200);
    resource->AddResponseHeader("Content-Type", "text/html");
    input_->AddResource(resource);
    return FakeDomDocument::New(iframe, url);
  }

  void CheckNoViolations() {
    std::vector<Violation> expected;
    CheckExpectedViolations(expected);
  }

  void CheckOneViolation(const std::string& violation_url,
                         int num_inline_style_blocks,
                         const std::vector<std::string>& external_styles) {
    std::vector<Violation> expected;
    expected.push_back(Violation(violation_url, num_inline_style_blocks,
                                 external_styles));
    CheckExpectedViolations(expected);
  }

  void CheckTwoViolations(const std::string& violation_url1,
                          int num_inline_style_blocks1,
                          const std::vector<std::string>& external_styles1,
                          const std::string& violation_url2,
                          int num_inline_style_blocks2,
                          const std::vector<std::string>& external_styles2) {
    std::vector<Violation> expected;
    expected.push_back(Violation(violation_url1, num_inline_style_blocks1,
                                 external_styles1));
    expected.push_back(Violation(violation_url2, num_inline_style_blocks2,
                                 external_styles2));
    CheckExpectedViolations(expected);
  }

  scoped_ptr<FakeDomDocument> document_;
  FakeDomElement* html_;
  FakeDomElement* head_;
  FakeDomElement* body_;

 private:
  struct Violation {
    Violation() : num_inline_style_blocks(0) {}
    Violation(const std::string& url_,
              int num_inline_style_blocks_,
              const std::vector<std::string>& external_styles_)
        : url(url_),
          num_inline_style_blocks(num_inline_style_blocks_),
          external_styles(external_styles_) {}
    std::string url;
    int num_inline_style_blocks;
    std::vector<std::string> external_styles;
  };

  void CheckExpectedViolations(const std::vector<Violation>& expected) {
    input_->AcquireDomDocument(document_.get());
    Freeze();

    pagespeed::rules::PutCssInTheDocumentHead put_css_in_head_rule;

    pagespeed::Results results;
    pagespeed::ResultProvider provider(put_css_in_head_rule, &results);
    ASSERT_TRUE(put_css_in_head_rule.AppendResults(*input_, &provider));
    ASSERT_EQ(static_cast<size_t>(results.results_size()), expected.size());

    for (size_t i = 0; i < expected.size(); ++i) {
      const pagespeed::Result& result = results.results(i);
      const Violation& violation = expected[i];
      ASSERT_EQ(result.resource_urls_size(), 1);
      ASSERT_EQ(violation.url, result.resource_urls(0));
      const pagespeed::ResultDetails& details = result.details();
      ASSERT_TRUE(details.HasExtension(
          StylesInBodyDetails::message_set_extension));
      const StylesInBodyDetails& style_details = details.GetExtension(
          StylesInBodyDetails::message_set_extension);
      ASSERT_EQ(violation.num_inline_style_blocks,
                style_details.num_inline_style_blocks());
      ASSERT_EQ(violation.external_styles.size(),
                static_cast<size_t>(style_details.external_styles_size()));
      for (size_t j = 0; j < violation.external_styles.size(); ++j) {
        ASSERT_EQ(violation.external_styles[j],
                  style_details.external_styles(j));
      }
    }
  }
};

const char* PutCssInTheDocumentHeadTest::kRootUrl = "http://example.com/";

TEST_F(PutCssInTheDocumentHeadTest, Empty) {
  CheckNoViolations();
}

TEST_F(PutCssInTheDocumentHeadTest, StylesInHead) {
  FakeDomElement::NewLinkStylesheet(head_, "http://example.com/foo.css");
  FakeDomElement::NewStyle(head_);
  CheckNoViolations();
}

TEST_F(PutCssInTheDocumentHeadTest, StyleTagInBody) {
  FakeDomElement::NewLinkStylesheet(head_, "http://example.com/foo.css");
  FakeDomElement::NewStyle(head_);
  FakeDomElement::NewStyle(body_);
  std::vector<std::string> external_styles;
  CheckOneViolation("http://example.com/", 1, external_styles);
}

TEST_F(PutCssInTheDocumentHeadTest, LinkTagInBody) {
  FakeDomElement::NewLinkStylesheet(head_, "http://example.com/foo.css");
  FakeDomElement::NewStyle(head_);
  FakeDomElement::NewLinkStylesheet(body_, "http://example.com/bar.css");
  std::vector<std::string> external_styles;
  external_styles.push_back("http://example.com/bar.css");
  CheckOneViolation("http://example.com/", 0, external_styles);
}

TEST_F(PutCssInTheDocumentHeadTest, SeveralThingsInBody) {
  FakeDomElement::NewLinkStylesheet(head_, "http://example.com/foo.css");
  FakeDomElement::NewStyle(head_);
  FakeDomElement::NewStyle(body_);
  FakeDomElement::NewLinkStylesheet(body_, "http://example.com/bar.css");
  FakeDomElement::NewStyle(body_);
  FakeDomElement::NewLinkStylesheet(body_, "http://example.com/baz.css");
  FakeDomElement::NewStyle(body_);
  std::vector<std::string> external_styles;
  external_styles.push_back("http://example.com/bar.css");
  external_styles.push_back("http://example.com/baz.css");
  CheckOneViolation("http://example.com/", 3, external_styles);
}

TEST_F(PutCssInTheDocumentHeadTest, Iframe) {
  // Main document:
  FakeDomElement::NewStyle(body_);
  FakeDomElement::NewStyle(body_);
  FakeDomElement::NewLinkStylesheet(body_, "http://example.com/bar.css");
  FakeDomElement* iframe = FakeDomElement::NewIframe(body_);

  // Iframe document:
  FakeDomDocument* doc2 =
      NewFakeDomDocument(iframe, "http://example.com/if.html");
  FakeDomElement* html2 = FakeDomElement::NewRoot(doc2, "HTML");
  FakeDomElement::New(html2, "HEAD");
  FakeDomElement* body2 = FakeDomElement::New(html2, "BODY");
  FakeDomElement::NewLinkStylesheet(body2, "http://example.com/foo.css");
  FakeDomElement::NewStyle(body2);
  std::vector<std::string> external_styles2;
  external_styles2.push_back("http://example.com/foo.css");

  std::vector<std::string> external_styles1;
  external_styles1.push_back("http://example.com/bar.css");

  CheckTwoViolations("http://example.com/if.html", 1, external_styles2,
                     "http://example.com/", 2, external_styles1);
}

}  // namespace
