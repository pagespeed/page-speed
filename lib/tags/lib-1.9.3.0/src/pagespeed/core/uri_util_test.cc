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

#include "pagespeed/core/uri_util.h"
#include "pagespeed/testing/pagespeed_test.h"

namespace {

using pagespeed_testing::FakeDomDocument;
using pagespeed_testing::FakeDomElement;

class ResolveUriForDocumentWithUrlTest
    : public ::pagespeed_testing::PagespeedTest {
 protected:
  static const char* kRootUrl;

  virtual void DoSetUp() {
    NewPrimaryResource(kRootUrl);
    CreateHtmlHeadBodyElements();
  }
};

const char* ResolveUriForDocumentWithUrlTest::kRootUrl =
    "http://example.com/testing/index.html";

TEST(UriUtilTest, ResolveUri) {
  ASSERT_EQ("http://www.example.com/foo",
            pagespeed::uri_util::ResolveUri("foo", "http://www.example.com/"));

  // Make sure that attempting to resolve an absolute URL returns that
  // absolute URL.
  ASSERT_EQ("http://www.testing.com/foo",
            pagespeed::uri_util::ResolveUri(
                "http://www.testing.com/foo", "http://www.example.com/"));
}

TEST_F(ResolveUriForDocumentWithUrlTest, FailsNullDocument) {
  std::string out;
  ASSERT_FALSE(pagespeed::uri_util::ResolveUriForDocumentWithUrl(
      "foo", NULL, kRootUrl, &out));
}

TEST_F(ResolveUriForDocumentWithUrlTest, FailsNoMatchingDocument) {
  std::string out;
  ASSERT_FALSE(pagespeed::uri_util::ResolveUriForDocumentWithUrl(
      "foo", document(), "http://example.com/testing/foo.html", &out));
}

TEST_F(ResolveUriForDocumentWithUrlTest, Basic) {
  std::string out;
  ASSERT_TRUE(pagespeed::uri_util::ResolveUriForDocumentWithUrl(
      "foo", document(), kRootUrl, &out));
  ASSERT_EQ("http://example.com/testing/foo", out);

  // Now override the base URL and make sure that the URI is resolved
  // relative to it.
  document()->SetBaseUrl("http://testing.com/foo/");
  ASSERT_TRUE(pagespeed::uri_util::ResolveUriForDocumentWithUrl(
      "foo", document(), kRootUrl, &out));
  ASSERT_EQ("http://testing.com/foo/foo", out);
}

TEST_F(ResolveUriForDocumentWithUrlTest, Iframe) {
  const char* kFrameUrl = "http://example.com/iframe/";
  FakeDomElement* iframe = FakeDomElement::NewIframe(body());
  FakeDomDocument* iframe_doc;
  NewDocumentResource(kFrameUrl, iframe, &iframe_doc);

  std::string out;
  ASSERT_TRUE(pagespeed::uri_util::ResolveUriForDocumentWithUrl(
      "foo", document(), kFrameUrl, &out));
  ASSERT_EQ("http://example.com/iframe/foo", out);

  // Now override the base URL and make sure that the URI is resolved
  // relative to it.
  iframe_doc->SetBaseUrl("http://testing.com/foo/iframe/");
  ASSERT_TRUE(pagespeed::uri_util::ResolveUriForDocumentWithUrl(
      "foo", document(), kFrameUrl, &out));
  ASSERT_EQ("http://testing.com/foo/iframe/foo", out);
}

}  // namespace
