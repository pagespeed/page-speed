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

#include "pagespeed/core/pagespeed_input.h"
#include "pagespeed/core/resource.h"
#include "pagespeed/testing/pagespeed_test.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

using pagespeed::PagespeedInput;
using pagespeed_testing::FakeDomDocument;
using pagespeed_testing::FakeDomElement;

static const char* kURL1 = "http://www.foo.com/";
static const char* kURL2 = "http://www.bar.com/";

pagespeed::Resource* NewResource(const std::string& url, int status_code) {
  pagespeed::Resource* resource = new pagespeed::Resource;
  resource->SetRequestUrl(url);
  resource->SetResponseStatusCode(status_code);
  return resource;
}

TEST(PagespeedInputTest, DisallowDuplicates) {
  pagespeed::PagespeedInput input;

  EXPECT_TRUE(input.AddResource(NewResource(kURL1, 200)));
  EXPECT_TRUE(input.AddResource(NewResource(kURL2, 200)));
  EXPECT_FALSE(input.AddResource(NewResource(kURL2, 200)));
  ASSERT_TRUE(input.Freeze());
  ASSERT_EQ(input.num_resources(), 2);
  EXPECT_EQ(input.GetResource(0).GetRequestUrl(), kURL1);
  EXPECT_EQ(input.GetResource(1).GetRequestUrl(), kURL2);
}

TEST(PagespeedInputTest, AllowDuplicates) {
  pagespeed::PagespeedInput input;
  input.set_allow_duplicate_resources();

  EXPECT_TRUE(input.AddResource(NewResource(kURL1, 200)));
  EXPECT_TRUE(input.AddResource(NewResource(kURL2, 200)));
  EXPECT_TRUE(input.AddResource(NewResource(kURL2, 200)));
  ASSERT_TRUE(input.Freeze());
  ASSERT_EQ(input.num_resources(), 3);
  EXPECT_EQ(input.GetResource(0).GetRequestUrl(), kURL1);
  EXPECT_EQ(input.GetResource(1).GetRequestUrl(), kURL2);
  EXPECT_EQ(input.GetResource(2).GetRequestUrl(), kURL2);
}

TEST(PagespeedInputTest, FilterBadResources) {
  pagespeed::PagespeedInput input;
  EXPECT_FALSE(input.AddResource(NewResource("", 0)));
  EXPECT_FALSE(input.AddResource(NewResource("", 200)));
  EXPECT_FALSE(input.AddResource(NewResource(kURL1, 0)));
  EXPECT_FALSE(input.AddResource(NewResource(kURL1, -1)));
  ASSERT_TRUE(input.Freeze());
}

TEST(PagespeedInputTest, FilterResources) {
  pagespeed::PagespeedInput input(
      new pagespeed::NotResourceFilter(new pagespeed::AllowAllResourceFilter));
  EXPECT_FALSE(input.AddResource(NewResource(kURL1, 200)));
  ASSERT_TRUE(input.Freeze());
}

class UpdateResourceTypesTest : public pagespeed_testing::PagespeedTest {
 protected:
  static const char* kRootUrl;

  virtual void DoSetUp() {
    NewPrimaryResource(kRootUrl);
    CreateHtmlHeadBodyElements();
  }
};

const char* UpdateResourceTypesTest::kRootUrl = "http://example.com/";

TEST_F(UpdateResourceTypesTest, Script) {
  pagespeed::Resource* resource =
      NewScriptResource("http://example.com/foo.js", body());
  resource->SetResourceType(pagespeed::OTHER);
  resource->AddResponseHeader("content-type", "text/html");
  ASSERT_EQ(pagespeed::HTML, resource->GetResourceType());
  Freeze();
  ASSERT_EQ(pagespeed::JS, resource->GetResourceType());
}

TEST_F(UpdateResourceTypesTest, Img) {
  pagespeed::Resource* resource =
      NewPngResource("http://example.com/foo.png", body());
  resource->SetResourceType(pagespeed::OTHER);
  resource->RemoveResponseHeader("content-type");
  resource->AddResponseHeader("content-type", "text/html");
  ASSERT_EQ(pagespeed::HTML, resource->GetResourceType());
  Freeze();
  ASSERT_EQ(pagespeed::IMAGE, resource->GetResourceType());
}

TEST_F(UpdateResourceTypesTest, Stylesheet) {
  pagespeed::Resource* resource =
      NewCssResource("http://example.com/foo.css", body());
  resource->SetResourceType(pagespeed::OTHER);
  resource->AddResponseHeader("content-type", "text/html");
  ASSERT_EQ(pagespeed::HTML, resource->GetResourceType());
  Freeze();
  ASSERT_EQ(pagespeed::CSS, resource->GetResourceType());
}

TEST_F(UpdateResourceTypesTest, Iframe) {
  FakeDomElement* iframe = FakeDomElement::NewIframe(body());
  pagespeed::Resource* resource =
      NewDocumentResource("http://example.com/iframe.html", iframe);
  resource->SetResourceType(pagespeed::OTHER);
  ASSERT_EQ(pagespeed::OTHER, resource->GetResourceType());
  Freeze();
  ASSERT_EQ(pagespeed::HTML, resource->GetResourceType());
}

TEST_F(UpdateResourceTypesTest, StylesheetInIframe) {
  FakeDomElement* iframe = FakeDomElement::NewIframe(body());
  FakeDomDocument* document =
      FakeDomDocument::New(iframe, "http://example.com/iframe.html");
  FakeDomElement* html = FakeDomElement::NewRoot(document, "html");

  // Add a resource in the iframe.
  pagespeed::Resource* resource =
      NewCssResource("http://example.com/foo.css", html);
  resource->SetResourceType(pagespeed::OTHER);
  resource->AddResponseHeader("content-type", "text/html");

  ASSERT_EQ(pagespeed::HTML, resource->GetResourceType());
  Freeze();
  ASSERT_EQ(pagespeed::CSS, resource->GetResourceType());
}

TEST_F(UpdateResourceTypesTest, DifferentTypesSameUrl) {
  // Create two different types of nodes in the DOM, one stylesheet
  // and one script, with the same URL. The resource type chosen by
  // the DOM visitor should be the first resource type that appears in
  // the DOM (in this case, stylesheet).

  // First add the stylesheet resource and node.
  pagespeed::Resource* resource =
      NewCssResource("http://example.com/foo", body());
  resource->SetResourceType(pagespeed::OTHER);
  resource->AddResponseHeader("content-type", "text/html");

  // Next add a script node with the same URL.
  FakeDomElement::NewScript(body(), "http://example.com/foo");
  ASSERT_EQ(pagespeed::HTML, resource->GetResourceType());
  Freeze();

  // Verify that the chosen type matches the first node type:
  // stylesheet.
  ASSERT_EQ(pagespeed::CSS, resource->GetResourceType());
}

class ParentChildResourceMapTest : public pagespeed_testing::PagespeedTest {
 protected:
  static const char* kRootUrl;

  virtual void DoSetUp() {
    NewPrimaryResource(kRootUrl);
    CreateHtmlHeadBodyElements();
  }
};

const char* ParentChildResourceMapTest::kRootUrl = "http://example.com/";

// TESTS: basic, iframe, some missing

TEST_F(ParentChildResourceMapTest, Basic) {
  pagespeed::Resource* css =
      NewCssResource("http://example.com/css.css", body());
  pagespeed::Resource* js1 =
      NewScriptResource("http://example.com/script1.js", body());
  pagespeed::Resource* js2 =
      NewScriptResource("http://example.com/script2.js", body());
  Freeze();

  // Validate that the parent child resource map was populated with
  // the expected contents.
  pagespeed::ParentChildResourceMap expected;
  expected[primary_resource()].push_back(css);
  expected[primary_resource()].push_back(js1);
  expected[primary_resource()].push_back(js2);
  ASSERT_TRUE(expected == *input()->GetParentChildResourceMap());
}

TEST_F(ParentChildResourceMapTest, Iframes) {
  pagespeed::Resource* js =
      NewScriptResource("http://example.com/script.js", body());
  FakeDomElement* iframe1 = FakeDomElement::NewIframe(body());
  FakeDomDocument* iframe1_doc;
  pagespeed::Resource* iframe1_resource =
      NewDocumentResource("http://example.com/iframe.html",
                          iframe1, &iframe1_doc);
  FakeDomElement* iframe1_root = FakeDomElement::NewRoot(iframe1_doc, "html");
  pagespeed::Resource* css =
      NewCssResource("http://example.com/css.css", iframe1_root);
  FakeDomElement::NewScript(iframe1_root, "http://example.com/script.js");
  FakeDomElement::NewScript(iframe1_root, "http://example.com/script.js");

  FakeDomElement* iframe2 = FakeDomElement::NewIframe(body());
  FakeDomDocument* iframe2_doc;
  pagespeed::Resource* iframe2_resource =
      NewDocumentResource("http://example.com/iframe2.html",
                          iframe2, &iframe2_doc);
  FakeDomElement* iframe2_root = FakeDomElement::NewRoot(iframe2_doc, "html");
  FakeDomElement::NewLinkStylesheet(iframe2_root, "http://example.com/css.css");
  FakeDomElement::NewScript(iframe2_root, "http://example.com/script.js");

  FakeDomElement* iframe3 = FakeDomElement::NewIframe(iframe2_root);
  FakeDomDocument* iframe3_doc;
  pagespeed::Resource* iframe3_resource =
      NewDocumentResource("http://example.com/iframe3.html",
                          iframe3, &iframe3_doc);
  FakeDomElement* iframe3_root = FakeDomElement::NewRoot(iframe3_doc, "html");
  FakeDomElement::NewLinkStylesheet(iframe3_root, "http://example.com/css.css");
  pagespeed::Resource* css2 =
      NewCssResource("http://example.com/css2.css", iframe3_root);
  Freeze();
  ASSERT_EQ(7, input()->num_resources());

  // Validate that the parent child resource map was populated with
  // the expected contents.
  pagespeed::ParentChildResourceMap expected;
  expected[primary_resource()].push_back(js);
  expected[primary_resource()].push_back(iframe1_resource);
  expected[iframe1_resource].push_back(css);
  expected[iframe1_resource].push_back(js);
  expected[primary_resource()].push_back(iframe2_resource);
  expected[iframe2_resource].push_back(css);
  expected[iframe2_resource].push_back(js);
  expected[iframe2_resource].push_back(iframe3_resource);
  expected[iframe3_resource].push_back(css);
  expected[iframe3_resource].push_back(css2);
  ASSERT_TRUE(expected == *input()->GetParentChildResourceMap());
}

TEST_F(ParentChildResourceMapTest, MissingResource) {
  FakeDomElement* iframe1 = FakeDomElement::NewIframe(body());
  FakeDomDocument* iframe1_doc;
  pagespeed::Resource* iframe1_resource =
      NewDocumentResource("http://example.com/iframe.html",
                          iframe1, &iframe1_doc);
  FakeDomElement* iframe1_root = FakeDomElement::NewRoot(iframe1_doc, "html");
  pagespeed::Resource* css =
      NewCssResource("http://example.com/css.css", iframe1_root);
  pagespeed::Resource* js =
      NewScriptResource("http://example.com/script.js", iframe1_root);

  FakeDomElement* iframe2 = FakeDomElement::NewIframe(body());

  // Create a document element inside the iframe, but do not create a
  // corresponding Resource for that document element. We expect that
  // the parent->child mapper will fail to find this frame or any of
  // its resources, since the document's resource is missing.
  FakeDomDocument* iframe2_doc =
      FakeDomDocument::New(iframe2, "http://example.com/iframe2.html");
  FakeDomElement* iframe2_root = FakeDomElement::NewRoot(iframe2_doc, "html");
  FakeDomElement::NewLinkStylesheet(iframe2_root, "http://example.com/css.css");
  FakeDomElement::NewScript(iframe2_root, "http://example.com/script.js");

  // This frame and one of its children should be found, since there
  // is a corresponding Resource for the document node.
  FakeDomElement* iframe3 = FakeDomElement::NewIframe(iframe2_root);
  FakeDomDocument* iframe3_doc;
  pagespeed::Resource* iframe3_resource =
      NewDocumentResource("http://example.com/iframe3.html",
                          iframe3, &iframe3_doc);
  FakeDomElement* iframe3_root = FakeDomElement::NewRoot(iframe3_doc, "html");
  FakeDomElement::NewLinkStylesheet(iframe3_root, "http://example.com/css.css");

  // Create a link element for which there is no corresponding
  // Resource. We do not expect a resource for this node to show up in
  // the map.
  FakeDomElement::NewLinkStylesheet(
      iframe3_root, "http://example.com/css2.css");

  Freeze();
  ASSERT_EQ(5, input()->num_resources());

  // Validate that the parent child resource map was populated with
  // the expected contents.
  pagespeed::ParentChildResourceMap expected;
  expected[primary_resource()].push_back(iframe1_resource);
  expected[iframe1_resource].push_back(css);
  expected[iframe1_resource].push_back(js);
  expected[iframe3_resource].push_back(css);
  ASSERT_TRUE(expected == *input()->GetParentChildResourceMap());
}

}  // namespace
