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

#include "googleurl/src/gurl.h"
#include "pagespeed/core/uri_util.h"
#include "pagespeed/testing/pagespeed_test.h"

namespace {

using pagespeed_testing::FakeDomDocument;
using pagespeed_testing::FakeDomElement;
using pagespeed::uri_util::GetDomainAndRegistry;
using pagespeed::uri_util::GetHost;
using pagespeed::uri_util::GetPath;

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

TEST(UriUtilTest, IsExternalResourceUrl) {
  ASSERT_FALSE(pagespeed::uri_util::IsExternalResourceUrl(
      "data:image/png;base64,iVBORw0KGgoAA"));
  ASSERT_TRUE(pagespeed::uri_util::IsExternalResourceUrl(
      "http://www.example.com/"));
  ASSERT_TRUE(pagespeed::uri_util::IsExternalResourceUrl(
      "https://www.example.com/foo.js"));
}

// Basic test to make sure we properly process UTF8 characters in
// URLs.
TEST(UriUtilTest, Utf8) {
  const char *kUtf8Url = "http://www.example.com/Résumé.html?q=Résumé";
  GURL gurl(kUtf8Url);
  ASSERT_TRUE(gurl.is_valid());
  ASSERT_EQ("http://www.example.com/R%C3%A9sum%C3%A9.html?q=R%C3%A9sum%C3%A9",
            gurl.spec());
}

TEST(UriUtilTest, GetUriWithoutFragmentTest) {
  static const char* kNoFragmentUrl = "http://www.example.com/foo";
  static const char* kFragmentUrl = "http://www.example.com/foo#fragment";
  static const char* kFragmentUrlNoFragment = "http://www.example.com/foo";
  std::string uri_no_fragment;
  ASSERT_TRUE(pagespeed::uri_util::GetUriWithoutFragment(
      kNoFragmentUrl, &uri_no_fragment));
  ASSERT_EQ(uri_no_fragment, kNoFragmentUrl);

  ASSERT_TRUE(pagespeed::uri_util::GetUriWithoutFragment(
      kFragmentUrl, &uri_no_fragment));
  ASSERT_EQ(uri_no_fragment, kFragmentUrlNoFragment);

  ASSERT_FALSE(
      pagespeed::uri_util::GetUriWithoutFragment("", &uri_no_fragment));
}

TEST(UriUtilTest, CanonicalizeUrl) {
  std::string url = "http://www.foo.com";
  pagespeed::uri_util::CanonicalizeUrl(&url);
  ASSERT_EQ("http://www.foo.com/", url);
  pagespeed::uri_util::CanonicalizeUrl(&url);
  ASSERT_EQ("http://www.foo.com/", url);
}

TEST(UriUtilTest, GetDomainAndRegistry) {
  EXPECT_EQ("google.com",
            GetDomainAndRegistry("http://www.google.com/file.html"));
  EXPECT_EQ("google.com",
            GetDomainAndRegistry("http://..google.com/file.html"));
  EXPECT_EQ("google.com.",
            GetDomainAndRegistry("http://google.com./file.html"));
  EXPECT_EQ("", GetDomainAndRegistry("http://google.com../file.html"));
  EXPECT_EQ("b.co.uk", GetDomainAndRegistry("http://a.b.co.uk/file.html"));
  EXPECT_EQ("", GetDomainAndRegistry("file:///C:/bar.html"));
  EXPECT_EQ("", GetDomainAndRegistry("http://foo.com../file.html"));
  EXPECT_EQ("", GetDomainAndRegistry("http://192.168.0.1/file.html"));
  EXPECT_EQ("", GetDomainAndRegistry("http://bar/file.html"));
  EXPECT_EQ("", GetDomainAndRegistry("http://co.uk/file.html"));
  EXPECT_EQ("foo.bar", GetDomainAndRegistry("http://foo.bar/file.html"));

  EXPECT_EQ("", GetDomainAndRegistry("http://./file.html"));
  EXPECT_EQ("", GetDomainAndRegistry("http://../file.html"));
  EXPECT_EQ("", GetDomainAndRegistry("http://.a/file.html"));
  EXPECT_EQ("", GetDomainAndRegistry("http://a./file.html"));
  EXPECT_EQ("", GetDomainAndRegistry("http://.a./file.html"));
  EXPECT_EQ("", GetDomainAndRegistry("http://.a../file.html"));
  EXPECT_EQ("", GetDomainAndRegistry("http://a../file.html"));

  EXPECT_EQ("", GetDomainAndRegistry("http://"));
  EXPECT_EQ("", GetDomainAndRegistry("http:// "));
  EXPECT_EQ("", GetDomainAndRegistry("http://  "));
  EXPECT_EQ("", GetDomainAndRegistry("http://."));
  EXPECT_EQ("", GetDomainAndRegistry("http://.."));
  EXPECT_EQ("", GetDomainAndRegistry("http://..."));
  EXPECT_EQ("", GetDomainAndRegistry("http://. ."));
  EXPECT_EQ("", GetDomainAndRegistry("http://. . "));
  EXPECT_EQ("", GetDomainAndRegistry("http:// ."));
  EXPECT_EQ("", GetDomainAndRegistry("http:// . "));
}

TEST(UriUtilTest, GetHost) {
  EXPECT_EQ("", GetHost(""));
  EXPECT_EQ("", GetHost("www.example.com"));
  EXPECT_EQ("", GetHost("/abc?def"));
  EXPECT_EQ("www.example.com", GetHost("http://www.example.com/abc?def"));
  EXPECT_EQ("www.example.com", GetHost("http://www.example.com"));
}

TEST(UriUtilTest, GetPath) {
  EXPECT_EQ("", GetPath(""));
  EXPECT_EQ("", GetPath("/abc?def"));
  EXPECT_EQ("/abc?def", GetPath("http://www.example.com/abc?def"));
}

}  // namespace
