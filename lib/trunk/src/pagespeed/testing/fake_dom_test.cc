// Copyright 2010 Google Inc. All Rights Reserved.
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
#include "base/stl_util-inl.h"
#include "pagespeed/core/dom.h"
#include "pagespeed/testing/fake_dom.h"
#include "pagespeed/testing/pagespeed_test.h"

namespace {

using pagespeed::DomDocument;
using pagespeed::DomElement;
using pagespeed_testing::FakeDomDocument;
using pagespeed_testing::FakeDomElement;

class FakeDomElementVisitor : public pagespeed::DomElementVisitor {
 public:
  virtual void Visit(const pagespeed::DomElement& node) {
    tags_.push_back(node.GetTagName());
  }

  std::vector<std::string> tags_;
};

class FakeDomExternalResourceVisitor
    : public pagespeed::ExternalResourceDomElementVisitor {
 public:
  virtual void VisitUrl(
      const pagespeed::DomElement& node, const std::string& url) {
    urls_.push_back(url);
  }

  virtual void VisitDocument(const pagespeed::DomElement& element,
                             const pagespeed::DomDocument& document) {
    scoped_ptr<pagespeed::DomElementVisitor> visitor(
        pagespeed::MakeDomElementVisitorForDocument(&document, this));
    document.Traverse(visitor.get());
  }

  std::vector<std::string> urls_;
};

class FakeDomTest : public ::testing::Test {
 protected:
  static const char* kRootUrl;
  static const char* kChildUrl;

  virtual void SetUp() {
    document_.reset(FakeDomDocument::NewRoot(kRootUrl));
  }

  int num_visited_tags() const {
    return visitor_.tags_.size();
  }

  const std::string& visited_tag(int idx) {
    return visitor_.tags_[idx];
  }

  void ClearVisitedTags() {
    visitor_.tags_.clear();
  }

  scoped_ptr<FakeDomDocument> document_;
  FakeDomElementVisitor visitor_;
};

class FakeDomExternalResourceTest : public pagespeed_testing::PagespeedTest {
 protected:
  static const char* kRootUrl;
  static const char* kChild1Url;
  static const char* kChild2Url;

  virtual void DoSetUp() {
    NewPrimaryResource(kRootUrl);
    CreateHtmlHeadBodyElements();
  }

  void Traverse() {
    scoped_ptr<pagespeed::DomElementVisitor> visitor(
        pagespeed::MakeDomElementVisitorForDocument(document(), &visitor_));
    document()->Traverse(visitor.get());
  }

  int num_urls() const {
    return visitor_.urls_.size();
  }

  const std::string& url(int idx) {
    return visitor_.urls_[idx];
  }

  FakeDomExternalResourceVisitor visitor_;
};

class ChildrenVisitor : public pagespeed::DomElementVisitor {
 public:
  ChildrenVisitor() {}
  virtual void Visit(const DomElement& node);
  const std::vector<std::string>& children() const { return children_; }
 private:
  std::vector<std::string> children_;
  DISALLOW_COPY_AND_ASSIGN(ChildrenVisitor);
};

void ChildrenVisitor::Visit(const DomElement& node) {
  size_t size = 0;
  ASSERT_EQ(node.GetNumChildren(&size), DomElement::SUCCESS);
  for (size_t idx = 0; idx < size; ++idx) {
    const DomElement* child;
    ASSERT_EQ(node.GetChild(&child, idx), DomElement::SUCCESS);
    scoped_ptr<const DomElement> child_ptr(child);
    children_.push_back(child->GetTagName());
  }
  if (node.GetTagName() == "IFRAME") {
    scoped_ptr<DomDocument> subdoc(node.GetContentDocument());
    if (subdoc != NULL) {
      subdoc->Traverse(this);
    }
  }
}

const char* FakeDomTest::kRootUrl = "http://www.example.com/foo.html";
const char* FakeDomTest::kChildUrl = "http://www.foo.com/bar.html";
const char* FakeDomExternalResourceTest::kRootUrl =
    "http://www.example.com/foo.html";
const char* FakeDomExternalResourceTest::kChild1Url =
    "http://www.foo.com/bar.html";
const char* FakeDomExternalResourceTest::kChild2Url =
    "http://www.foo.com/somepath/bar.html";

TEST_F(FakeDomTest, Basic) {
  ASSERT_EQ(kRootUrl, document_->GetDocumentUrl());
  ASSERT_EQ(kRootUrl, document_->GetBaseUrl());
}

TEST_F(FakeDomTest, TraverseNoNodes) {
  document_->Traverse(&visitor_);
  ASSERT_EQ(0, num_visited_tags());
}

TEST_F(FakeDomTest, NewRootTwiceFails) {
  FakeDomElement* root = FakeDomElement::NewRoot(document_.get(), "html");
  ASSERT_EQ("HTML", root->GetTagName());

#ifdef NDEBUG
  ASSERT_EQ(NULL, FakeDomElement::NewRoot(document_.get(), "html"));
#else
  ASSERT_DEATH(FakeDomElement::NewRoot(document_.get(), "html"),
               "Document already has document element.");
#endif
}

TEST_F(FakeDomTest, NewDocumentFailsForNonIframe) {
  FakeDomElement* root = FakeDomElement::NewRoot(document_.get(), "html");
#ifdef NDEBUG
  ASSERT_EQ(NULL, FakeDomDocument::New(root, kChildUrl));
#else
  ASSERT_DEATH(FakeDomDocument::New(root, kChildUrl),
               "Unable to create document in non-iframe tag.");
#endif
}

TEST_F(FakeDomTest, NewDocumentFailsWhenIframeAlreadyHasDocument) {
  FakeDomElement* root = FakeDomElement::NewRoot(document_.get(), "html");
  FakeDomElement* body = FakeDomElement::New(root, "body");
  FakeDomElement* iframe = FakeDomElement::New(body, "iframe");
  FakeDomDocument* child = FakeDomDocument::New(iframe, kChildUrl);
  ASSERT_EQ(kChildUrl, child->GetDocumentUrl());

#ifdef NDEBUG
  ASSERT_EQ(NULL, FakeDomDocument::New(iframe, kChildUrl));
#else
  ASSERT_DEATH(FakeDomDocument::New(iframe, kChildUrl),
               "iframe already has child document.");
#endif
}

TEST_F(FakeDomTest, NoContentDocument) {
  FakeDomElement* root = FakeDomElement::NewRoot(document_.get(), "html");
#ifdef NDEBUG
  ASSERT_EQ(NULL, root->GetContentDocument());
#else
  ASSERT_DEATH(root->GetContentDocument(),
               "No content document for non-iframe element.");
#endif

  FakeDomElement* body = FakeDomElement::New(root, "body");
#ifdef NDEBUG
  ASSERT_EQ(NULL, body->GetContentDocument());
#else
  ASSERT_DEATH(body->GetContentDocument(),
               "No content document for non-iframe element.");
#endif

  FakeDomElement* iframe = FakeDomElement::New(body, "iframe");
  ASSERT_EQ(NULL, iframe->GetContentDocument());
}

TEST_F(FakeDomTest, GetContentDocument) {
  FakeDomElement* root = FakeDomElement::NewRoot(document_.get(), "html");
  FakeDomElement* iframe = FakeDomElement::NewIframe(root);
  FakeDomDocument::New(iframe, kChildUrl);
  scoped_ptr<pagespeed::DomDocument> document(iframe->GetContentDocument());
  ASSERT_NE(static_cast<pagespeed::DomDocument*>(NULL), document.get());
  ASSERT_EQ(kChildUrl, document->GetDocumentUrl());

  // Get a few more instances to verify that Clone() is behaving
  // properly.
  scoped_ptr<pagespeed::DomDocument> doc2(iframe->GetContentDocument());
  scoped_ptr<pagespeed::DomDocument> doc3(iframe->GetContentDocument());
}

TEST_F(FakeDomTest, TraverseRootNode) {
  FakeDomElement::NewRoot(document_.get(), "html");
  document_->Traverse(&visitor_);
  ASSERT_EQ(1, num_visited_tags());
  ASSERT_EQ("HTML", visited_tag(0));
}

TEST_F(FakeDomTest, TraverseSmallTree) {
  FakeDomElement* root = FakeDomElement::NewRoot(document_.get(), "html");
  FakeDomElement::New(root, "body");

  document_->Traverse(&visitor_);
  ASSERT_EQ(2, num_visited_tags());
  ASSERT_EQ("HTML", visited_tag(0));
  ASSERT_EQ("BODY", visited_tag(1));
}

TEST_F(FakeDomTest, TraverseChildDocument) {
  FakeDomElement* root = FakeDomElement::NewRoot(document_.get(), "html");
  FakeDomElement* body = FakeDomElement::New(root, "body");
  FakeDomElement* iframe = FakeDomElement::New(body, "iframe");
  FakeDomDocument* child = FakeDomDocument::New(iframe, kChildUrl);
  FakeDomElement* child_root = FakeDomElement::NewRoot(child, "html");
  FakeDomElement* child_body = FakeDomElement::New(child_root, "body");
  FakeDomElement::New(child_body, "div");
  FakeDomElement* child_p = FakeDomElement::New(child_body, "p");
  FakeDomElement* child_ul = FakeDomElement::New(child_body, "ul");
  FakeDomElement::New(child_p, "pre");
  FakeDomElement::New(child_ul, "li");
  FakeDomElement::New(child_ul, "foo");

  document_->Traverse(&visitor_);
  ASSERT_EQ(3, num_visited_tags());
  ASSERT_EQ("HTML", visited_tag(0));
  ASSERT_EQ("BODY", visited_tag(1));
  ASSERT_EQ("IFRAME", visited_tag(2));

  ClearVisitedTags();
  child->Traverse(&visitor_);
  ASSERT_EQ(8, num_visited_tags());
  ASSERT_EQ("HTML", visited_tag(0));
  ASSERT_EQ("BODY", visited_tag(1));
  ASSERT_EQ("DIV", visited_tag(2));
  ASSERT_EQ("P", visited_tag(3));
  ASSERT_EQ("PRE", visited_tag(4));
  ASSERT_EQ("UL", visited_tag(5));
  ASSERT_EQ("LI", visited_tag(6));
  ASSERT_EQ("FOO", visited_tag(7));
}

TEST_F(FakeDomTest, GetAttributeByName) {
  FakeDomElement* root = FakeDomElement::NewRoot(document_.get(), "html");
  root->AddAttribute("foo", "bar");
  root->AddAttribute("a", "b");
  root->AddAttribute("yes", "no");
  std::string value;
  ASSERT_TRUE(root->GetAttributeByName("FOO", &value));
  ASSERT_EQ("bar", value);
  ASSERT_TRUE(root->GetAttributeByName("a", &value));
  ASSERT_EQ("b", value);
  ASSERT_TRUE(root->GetAttributeByName("yEs", &value));
  ASSERT_EQ("no", value);
}

TEST_F(FakeDomExternalResourceTest, Basic) {
  FakeDomElement* script = NULL;
  NewScriptResource("http://www.example.com/script.js", body(), &script);
  // Make the DOM node's URL relative, to verify that we make URLs
  // absolute in the visitor.
  script->AddAttribute("src", "script.js");

  // Create an "inline" script with no URL.
  FakeDomElement::New(body(), "script");

  // Add an img tag with a data URI.
  FakeDomElement::NewImg(body(), "data:image/png;base64,ZZZZZ");

  // Add a second instance of the script resource.
  FakeDomElement::NewScript(body(), "http://www.example.com/script.js");

  Traverse();
  ASSERT_EQ(2, num_urls());
  ASSERT_EQ("http://www.example.com/script.js", url(0));
  ASSERT_EQ("http://www.example.com/script.js", url(1));
}

TEST_F(FakeDomExternalResourceTest, Iframes) {
  FakeDomElement* iframe1 = FakeDomElement::NewIframe(body());
  iframe1->AddAttribute("src", kChild1Url);
  FakeDomDocument* document1 = NULL;
  NewDocumentResource(kChild1Url, iframe1, &document1);

  // document2 is a srcless document, i.e. a friendly iframe.
  FakeDomElement* iframe2 = FakeDomElement::NewIframe(body());
  FakeDomDocument* document2 = FakeDomDocument::New(iframe2, "");
  document2->SetBaseUrl("http://www.example.com/");
  FakeDomElement* document2_root = FakeDomElement::NewRoot(document2, "html");
  FakeDomElement::NewScript(document2_root, "script2.js");

  FakeDomElement* iframe3 = FakeDomElement::NewIframe(iframe2);
  FakeDomDocument* document3 = NULL;
  NewDocumentResource(kChild2Url, iframe3, &document3);
  FakeDomElement* document3_root = FakeDomElement::NewRoot(document3, "html");
  FakeDomElement::NewLinkStylesheet(document3_root, "sheet.css");

  Traverse();
  ASSERT_EQ(4, num_urls());
  ASSERT_EQ(kChild1Url, url(0));
  ASSERT_EQ("http://www.example.com/script2.js", url(1));
  ASSERT_EQ(kChild2Url, url(2));
  ASSERT_EQ("http://www.foo.com/somepath/sheet.css", url(3));
}

TEST_F(FakeDomTest, ChildElements) {
  FakeDomElement* root = FakeDomElement::NewRoot(document_.get(), "html");
  FakeDomElement* head = FakeDomElement::New(root, "head");
  FakeDomElement::New(head, "title");
  FakeDomElement* body = FakeDomElement::New(root, "body");
  FakeDomElement::New(body, "h1");

  ChildrenVisitor visitor;
  document_->Traverse(&visitor);
  EXPECT_EQ(4u, visitor.children().size());
  EXPECT_EQ("HEAD", visitor.children()[0]);
  EXPECT_EQ("BODY", visitor.children()[1]);
  EXPECT_EQ("TITLE", visitor.children()[2]);
  EXPECT_EQ("H1", visitor.children()[3]);
}

}  // namespace
