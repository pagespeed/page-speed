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

namespace {

using pagespeed::DomDocument;
using pagespeed::StylesInBodyDetails;
using pagespeed_testing::FakeDomDocument;
using pagespeed_testing::FakeDomElement;
using pagespeed_testing::PagespeedRuleTest;

class PutCssInTheDocumentHeadTest
    : public PagespeedRuleTest<pagespeed::rules::PutCssInTheDocumentHead> {
 protected:
  static const char* kRootUrl;

  virtual void DoSetUp() {
    NewPrimaryResource(kRootUrl);
    CreateHtmlHeadBodyElements();
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
    Freeze();
    ASSERT_TRUE(AppendResults());
    ASSERT_EQ(static_cast<size_t>(num_results()), expected.size());

    for (size_t i = 0; i < expected.size(); ++i) {
      const pagespeed::Result& res = result(i);
      const Violation& violation = expected[i];
      ASSERT_EQ(res.resource_urls_size(), 1);
      ASSERT_EQ(violation.url, res.resource_urls(0));
      const pagespeed::ResultDetails& details = res.details();
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
  FakeDomElement::NewLinkStylesheet(head(), "http://example.com/foo.css");
  FakeDomElement::NewStyle(head());
  CheckNoViolations();
}

TEST_F(PutCssInTheDocumentHeadTest, StyleTagInBody) {
  FakeDomElement::NewLinkStylesheet(head(), "http://example.com/foo.css");
  FakeDomElement::NewStyle(head());
  FakeDomElement::NewStyle(body());
  std::vector<std::string> external_styles;
  CheckOneViolation(kRootUrl, 1, external_styles);
}

TEST_F(PutCssInTheDocumentHeadTest, LinkTagInBody) {
  FakeDomElement::NewLinkStylesheet(head(), "http://example.com/foo.css");
  FakeDomElement::NewStyle(head());
  FakeDomElement::NewLinkStylesheet(body(), "http://example.com/bar.css");
  std::vector<std::string> external_styles;
  external_styles.push_back("http://example.com/bar.css");
  CheckOneViolation(kRootUrl, 0, external_styles);
}

TEST_F(PutCssInTheDocumentHeadTest, SeveralThingsInBody) {
  FakeDomElement::NewLinkStylesheet(head(), "http://example.com/foo.css");
  FakeDomElement::NewStyle(head());
  FakeDomElement::NewStyle(body());
  FakeDomElement::NewLinkStylesheet(body(), "http://example.com/bar.css");
  FakeDomElement::NewStyle(body());
  FakeDomElement::NewLinkStylesheet(body(), "http://example.com/baz.css");
  FakeDomElement::NewStyle(body());
  std::vector<std::string> external_styles;
  external_styles.push_back("http://example.com/bar.css");
  external_styles.push_back("http://example.com/baz.css");
  CheckOneViolation(kRootUrl, 3, external_styles);
}

TEST_F(PutCssInTheDocumentHeadTest, Iframe) {
  // Main document:
  FakeDomElement::NewStyle(body());
  FakeDomElement::NewStyle(body());
  FakeDomElement::NewLinkStylesheet(body(), "http://example.com/bar.css");
  FakeDomElement* iframe = FakeDomElement::NewIframe(body());

  // Iframe document:
  FakeDomDocument* doc2;
  NewDocumentResource("http://example.com/if.html",
                      iframe,
                      &doc2);
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
                     kRootUrl, 2, external_styles1);
}

}  // namespace
