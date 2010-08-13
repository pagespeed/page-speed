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

}  // namespace
