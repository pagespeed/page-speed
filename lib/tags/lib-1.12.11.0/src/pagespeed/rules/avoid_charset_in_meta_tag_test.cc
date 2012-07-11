// Copyright 2012 Google Inc. All Rights Reserved.
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
#include "pagespeed/core/pagespeed_input.h"
#include "pagespeed/core/resource.h"
#include "pagespeed/core/result_provider.h"
#include "pagespeed/proto/pagespeed_output.pb.h"
#include "pagespeed/rules/avoid_charset_in_meta_tag.h"
#include "pagespeed/testing/pagespeed_test.h"

using pagespeed::rules::AvoidCharsetInMetaTag;
using pagespeed::PagespeedInput;
using pagespeed::Resource;
using pagespeed::Result;
using pagespeed::Results;
using pagespeed::ResultProvider;
using pagespeed_testing::PagespeedRuleTest;

namespace {

class AvoidCharsetInMetaTagTest
    : public PagespeedRuleTest<AvoidCharsetInMetaTag> {
 protected:
  void AddTestResource(const std::string& url,
                       const std::string& body) {
    AddTestResource(url, "", "", body);
  }

  void AddTestResource(const std::string &url,
                       const std::string &header_name,
                       const std::string &header_value,
                       const std::string &body) {
    Resource* resource = new Resource;
    resource->SetResponseStatusCode(200);
    resource->SetRequestUrl(url);
    if (!header_name.empty()) {
      resource->AddResponseHeader(header_name, header_value);
    }
    resource->SetResponseBody(body);
    AddResource(resource);
  }
};

const char* kHtmlMetaCacheControl =
    "<html><meta hTtP-eQuiV='cache-control' content='no-cache' /></html>";

const char* kHtmlMetaContentTypeNoCharset =
    "<html><meta hTtP-eQuiV='cOnTeNt-tYpE' content='text/html' /></html>";

const char* kHtmlMetaContentTypeDefaultCharset =
    "<html><meta hTtP-eQuiV='cOnTeNt-tYpE' content='text/html; "
    "chARseT=ISO-8859-1' /></html>";

const char* kHtmlMetaContentTypeNonDefaultCharset =
    "<html><meta hTtP-eQuiV='cOnTeNt-tYpE' content='text/html; "
    "chARseT=UTF-8' /></html>";

const char* kHtmlMetaContentTypeCharsetTwice =
    "<html><meta hTtP-eQuiV='cOnTeNt-tYpE' content='text/html; "
    "chARseT=UTF-8' /><meta hTtP-eQuiV='cOnTeNt-tYpE' content='text/html; "
    "chARseT=UTF-16' /></html>";

const char* kHtml5MetaCharset = "<html><meta charset='UTF-8' /></html>";

TEST_F(AvoidCharsetInMetaTagTest, HasMetaCharsetTag) {
  std::string meta_charset_content;
  int meta_charset_begin_line_number;

  ASSERT_FALSE(AvoidCharsetInMetaTag::HasMetaCharsetTag(
      kUrl1, "", &meta_charset_content, &meta_charset_begin_line_number));

  ASSERT_FALSE(AvoidCharsetInMetaTag::HasMetaCharsetTag(
      kUrl1, kHtmlMetaCacheControl,
      &meta_charset_content, &meta_charset_begin_line_number));

  ASSERT_FALSE(AvoidCharsetInMetaTag::HasMetaCharsetTag(
      kUrl1, kHtmlMetaContentTypeNoCharset,
      &meta_charset_content, &meta_charset_begin_line_number));

  ASSERT_TRUE(AvoidCharsetInMetaTag::HasMetaCharsetTag(
      kUrl1, kHtmlMetaContentTypeDefaultCharset,
      &meta_charset_content, &meta_charset_begin_line_number));
  ASSERT_EQ("ISO-8859-1", meta_charset_content);
  ASSERT_EQ(1, meta_charset_begin_line_number);

  ASSERT_TRUE(AvoidCharsetInMetaTag::HasMetaCharsetTag(
      kUrl1, kHtmlMetaContentTypeNonDefaultCharset,
      &meta_charset_content, &meta_charset_begin_line_number));
  ASSERT_EQ("UTF-8", meta_charset_content);
  ASSERT_EQ(1, meta_charset_begin_line_number);

  ASSERT_TRUE(AvoidCharsetInMetaTag::HasMetaCharsetTag(
      kUrl1, kHtmlMetaContentTypeCharsetTwice,
      &meta_charset_content, &meta_charset_begin_line_number));
  ASSERT_EQ("UTF-8", meta_charset_content);
  ASSERT_EQ(1, meta_charset_begin_line_number);
}

TEST_F(AvoidCharsetInMetaTagTest, EmptyBody) {
  AddTestResource(kUrl1, "");
  CheckNoViolations();
}

TEST_F(AvoidCharsetInMetaTagTest, DefaultCharset) {
  AddTestResource(kUrl1,
                  kHtmlMetaContentTypeDefaultCharset);
  CheckNoViolations();
}

TEST_F(AvoidCharsetInMetaTagTest, NonDefaultCharset) {
  AddTestResource(kUrl1,
                  kHtmlMetaContentTypeNonDefaultCharset);
  CheckOneUrlViolation(kUrl1);
}

TEST_F(AvoidCharsetInMetaTagTest,
       NonDefaultCharsetWithContentTypeResponseHeader) {
  AddTestResource(kUrl1,
                  "content-type",
                  "text/html",
                  kHtmlMetaContentTypeNonDefaultCharset);
  CheckOneUrlViolation(kUrl1);
}

TEST_F(AvoidCharsetInMetaTagTest,
       NonDefaultCharsetWithCharsetInHttpResponseHeader) {
  AddTestResource(kUrl1,
                  "content-type",
                  "text/html; charset=UTF-8",
                  kHtmlMetaContentTypeNonDefaultCharset);
  CheckNoViolations();
}

TEST_F(AvoidCharsetInMetaTagTest, NonDefaultCharsetNotHtmlContentType) {
  AddTestResource(kUrl1,
                  "content-type",
                  "text/plain",
                  kHtmlMetaContentTypeNonDefaultCharset);
  CheckNoViolations();
}

TEST_F(AvoidCharsetInMetaTagTest, Html5MetaCharset) {
  AddTestResource(kUrl1, kHtml5MetaCharset);
  CheckOneUrlViolation(kUrl1);
}

}  // namespace
