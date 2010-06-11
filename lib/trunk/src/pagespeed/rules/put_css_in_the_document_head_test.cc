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
#include "testing/gtest/include/gtest/gtest.h"

using pagespeed::DomDocument;
using pagespeed::StylesInBodyDetails;

namespace {

class MockElement : public pagespeed::DomElement {
 public:
  // MockElement takes ownership of content.
  MockElement(DomDocument* content,
              const std::string& tagname,
              const std::map<std::string, std::string>& attributes)
      : content_(content),
        tagname_(tagname),
        children_(),
        attributes_(attributes) {}
  virtual ~MockElement() {
    STLDeleteContainerPointers(children_.begin(), children_.end());
  }

  static MockElement* NewTag(const std::string& tagname) {
    std::map<std::string, std::string> attributes;
    return new MockElement(NULL, tagname, attributes);
  }

  // Creates and returns a MockElement (which takes ownership of content).
  static MockElement* NewIframe(DomDocument* content) {
    std::map<std::string, std::string> attributes;
    attributes["src"] = content->GetDocumentUrl();
    return new MockElement(content, "IFRAME", attributes);
  }

  static MockElement* NewLinkTag(const std::string& href) {
    std::map<std::string, std::string> attributes;
    attributes["rel"] = "stylesheet";
    attributes["href"] = href;
    return new MockElement(NULL, "LINK", attributes);
  }

  static MockElement* NewStyleTag() {
    std::map<std::string, std::string> attributes;
    return new MockElement(NULL, "STYLE", attributes);
  }

  // Ownership is transferred to the caller. May be NULL.
  virtual DomDocument* GetContentDocument() const {
    return content_.release();
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

  void AddChild(MockElement* child) {
    children_.push_back(child);
  }

  void Traverse(pagespeed::DomElementVisitor* visitor) {
    visitor->Visit(*this);
    for (std::vector<MockElement*>::const_iterator i = children_.begin(),
             end = children_.end(); i != end; ++i) {
      MockElement* child = *i;
      child->Traverse(visitor);
    }
  }

 private:
  mutable scoped_ptr<DomDocument> content_;
  std::string tagname_;
  std::vector<MockElement*> children_;
  std::map<std::string, std::string> attributes_;

  DISALLOW_COPY_AND_ASSIGN(MockElement);
};

class MockDocument : public pagespeed::DomDocument {
 public:
  MockDocument(const std::string& url, MockElement* root) :
      url_(url), root_(root) {}

  virtual std::string GetDocumentUrl() const { return url_; }

  virtual void Traverse(pagespeed::DomElementVisitor* visitor) const {
    root_->Traverse(visitor);
  }

 private:
  const std::string url_;
  scoped_ptr<MockElement> root_;

  DISALLOW_COPY_AND_ASSIGN(MockDocument);
};

class PutCssInTheDocumentHeadTest : public ::testing::Test {
 protected:
  virtual void SetUp() {
    input_.reset(new pagespeed::PagespeedInput);
  }

  virtual void TearDown() {
    input_.reset();
  }

  MockDocument* NewMockDocument (const std::string& url, MockElement* root) {
    pagespeed::Resource* resource = new pagespeed::Resource;
    resource->SetRequestUrl(url);
    resource->SetRequestMethod("GET");
    resource->SetResponseStatusCode(200);
    resource->AddResponseHeader("Content-Type", "text/html");
    input_->AddResource(resource);
    return new MockDocument(url, root);
  }

  void CheckNoViolations(MockDocument* document) {
    std::vector<Violation> expected;
    CheckExpectedViolations(document, expected);
  }

  void CheckOneViolation(MockDocument* document,
                         const std::string& violation_url,
                         int num_inline_style_blocks,
                         const std::vector<std::string>& external_styles) {
    std::vector<Violation> expected;
    expected.push_back(Violation(violation_url, num_inline_style_blocks,
                                 external_styles));
    CheckExpectedViolations(document, expected);
  }

  void CheckTwoViolations(MockDocument* document,
                          const std::string& violation_url1,
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
    CheckExpectedViolations(document, expected);
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

  void CheckExpectedViolations(MockDocument* document,
                               const std::vector<Violation>& expected) {
    input_->AcquireDomDocument(document);

    pagespeed::rules::PutCssInTheDocumentHead put_css_in_head_rule;

    pagespeed::Results results;
    pagespeed::ResultProvider provider(put_css_in_head_rule, &results);
    ASSERT_TRUE(put_css_in_head_rule.AppendResults(*input_, &provider));
    ASSERT_EQ(results.results_size(), expected.size());

    for (int i = 0; i < expected.size(); ++i) {
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
                style_details.external_styles_size());
      for (int j = 0; j < violation.external_styles.size(); ++j) {
        ASSERT_EQ(violation.external_styles[j],
                  style_details.external_styles(j));
      }
    }
  }

  scoped_ptr<pagespeed::PagespeedInput> input_;
};

TEST_F(PutCssInTheDocumentHeadTest, Empty) {
  MockElement* html = MockElement::NewTag("HTML");
  html->AddChild(MockElement::NewTag("HEAD"));
  html->AddChild(MockElement::NewTag("BODY"));
  MockDocument* doc = NewMockDocument("http://example.com/", html);
  CheckNoViolations(doc);
}

TEST_F(PutCssInTheDocumentHeadTest, StylesInHead) {
  MockElement* html = MockElement::NewTag("HTML");
  MockElement* head = MockElement::NewTag("HEAD");
  head->AddChild(MockElement::NewLinkTag("http://example.com/foo.css"));
  head->AddChild(MockElement::NewStyleTag());
  html->AddChild(head);
  html->AddChild(MockElement::NewTag("BODY"));
  MockDocument* doc = NewMockDocument("http://example.com/", html);
  CheckNoViolations(doc);
}

TEST_F(PutCssInTheDocumentHeadTest, StyleTagInBody) {
  MockElement* html = MockElement::NewTag("HTML");
  MockElement* head = MockElement::NewTag("HEAD");
  head->AddChild(MockElement::NewLinkTag("http://example.com/foo.css"));
  head->AddChild(MockElement::NewStyleTag());
  html->AddChild(head);
  MockElement* body = MockElement::NewTag("BODY");
  body->AddChild(MockElement::NewStyleTag());
  html->AddChild(body);
  MockDocument* doc = NewMockDocument("http://example.com/", html);
  std::vector<std::string> external_styles;
  CheckOneViolation(doc, "http://example.com/", 1, external_styles);
}

TEST_F(PutCssInTheDocumentHeadTest, LinkTagInBody) {
  MockElement* html = MockElement::NewTag("HTML");
  MockElement* head = MockElement::NewTag("HEAD");
  head->AddChild(MockElement::NewLinkTag("http://example.com/foo.css"));
  head->AddChild(MockElement::NewStyleTag());
  html->AddChild(head);
  MockElement* body = MockElement::NewTag("BODY");
  body->AddChild(MockElement::NewLinkTag("http://example.com/bar.css"));
  html->AddChild(body);
  MockDocument* doc = NewMockDocument("http://example.com/", html);
  std::vector<std::string> external_styles;
  external_styles.push_back("http://example.com/bar.css");
  CheckOneViolation(doc, "http://example.com/", 0, external_styles);
}

TEST_F(PutCssInTheDocumentHeadTest, SeveralThingsInBody) {
  MockElement* html = MockElement::NewTag("HTML");
  MockElement* head = MockElement::NewTag("HEAD");
  head->AddChild(MockElement::NewLinkTag("http://example.com/foo.css"));
  head->AddChild(MockElement::NewStyleTag());
  html->AddChild(head);
  MockElement* body = MockElement::NewTag("BODY");
  body->AddChild(MockElement::NewStyleTag());
  body->AddChild(MockElement::NewLinkTag("http://example.com/bar.css"));
  body->AddChild(MockElement::NewStyleTag());
  body->AddChild(MockElement::NewLinkTag("http://example.com/baz.css"));
  body->AddChild(MockElement::NewStyleTag());
  html->AddChild(body);
  MockDocument* doc = NewMockDocument("http://example.com/", html);
  std::vector<std::string> external_styles;
  external_styles.push_back("http://example.com/bar.css");
  external_styles.push_back("http://example.com/baz.css");
  CheckOneViolation(doc, "http://example.com/", 3, external_styles);
}

TEST_F(PutCssInTheDocumentHeadTest, Iframe) {
  // Iframe document:
  MockElement* html2 = MockElement::NewTag("HTML");
  html2->AddChild(MockElement::NewTag("HEAD"));
  MockElement* body2 = MockElement::NewTag("BODY");
  body2->AddChild(MockElement::NewLinkTag("http://example.com/foo.css"));
  body2->AddChild(MockElement::NewStyleTag());
  html2->AddChild(body2);
  MockDocument* doc2 = NewMockDocument("http://example.com/if.html", html2);
  std::vector<std::string> external_styles2;
  external_styles2.push_back("http://example.com/foo.css");

  // Main document:
  MockElement* html1 = MockElement::NewTag("HTML");
  html1->AddChild(MockElement::NewTag("HEAD"));
  MockElement* body1 = MockElement::NewTag("BODY");
  body1->AddChild(MockElement::NewStyleTag());
  body1->AddChild(MockElement::NewStyleTag());
  body1->AddChild(MockElement::NewLinkTag("http://example.com/bar.css"));
  body1->AddChild(MockElement::NewIframe(doc2));
  html1->AddChild(body1);
  MockDocument* doc1 = NewMockDocument("http://example.com/", html1);
  std::vector<std::string> external_styles1;
  external_styles1.push_back("http://example.com/bar.css");

  CheckTwoViolations(doc1, "http://example.com/if.html", 1, external_styles2,
                     "http://example.com/", 2, external_styles1);
}

}  // namespace
