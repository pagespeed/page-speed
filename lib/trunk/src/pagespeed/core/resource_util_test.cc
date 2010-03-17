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

#include "pagespeed/core/resource.h"
#include "pagespeed/core/resource_util.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

namespace resource_util = pagespeed::resource_util;

class ResourceUtilTest : public testing::Test {
 protected:
  virtual void SetUp() {
    r_.SetRequestUrl("http://www.example.com/");
    r_.SetRequestMethod("GET");
    r_.SetResponseStatusCode(200);
  }

  pagespeed::Resource r_;
};

class HeaderDirectiveTest : public testing::Test {
 protected:
  void AssertBadHeaderDirectives(const char* header) {
    m_.clear();
    ASSERT_FALSE(resource_util::GetHeaderDirectives(header, &m_));
    ASSERT_TRUE(m_.empty());
  }

  void AssertEmptyHeaderDirectives(const char* header) {
    m_.clear();
    ASSERT_TRUE(resource_util::GetHeaderDirectives(header, &m_));
    ASSERT_TRUE(m_.empty());
  }

  void AssertOneHeaderDirective(const char* header,
                          const char* key,
                          const char* value) {
    m_.clear();
    ASSERT_TRUE(resource_util::GetHeaderDirectives(header, &m_));
    ASSERT_EQ(1, m_.size());
    resource_util::DirectiveMap::const_iterator it = m_.find(key);
    ASSERT_TRUE(m_.end() != it);
    ASSERT_STREQ(value, it->second.c_str());
  }

  void AssertTwoHeaderDirectives(const char* header,
                           const char* key1,
                           const char* value1,
                           const char* key2,
                           const char* value2) {
    m_.clear();
    ASSERT_TRUE(resource_util::GetHeaderDirectives(header, &m_));
    ASSERT_EQ(2, m_.size());
    resource_util::DirectiveMap::const_iterator it = m_.find(key1);
    ASSERT_TRUE(m_.end() != it);
    ASSERT_STREQ(value1, it->second.c_str());
    it = m_.find(key2);
    ASSERT_TRUE(m_.end() != it);
    ASSERT_STREQ(value2, it->second.c_str());
  }

  resource_util::DirectiveMap m_;
};

TEST_F(HeaderDirectiveTest, EmptyHeaderDirectives) {
  AssertEmptyHeaderDirectives("");
  AssertEmptyHeaderDirectives("   ");
  AssertEmptyHeaderDirectives(",");
  AssertEmptyHeaderDirectives(",,,,");
  AssertEmptyHeaderDirectives(" , , , , ");
  AssertEmptyHeaderDirectives(";");
  AssertEmptyHeaderDirectives(";;;;");
  AssertEmptyHeaderDirectives(" ; ; ; ; ");
}

TEST_F(HeaderDirectiveTest, OneHeaderDirective) {
  AssertOneHeaderDirective("foo", "foo", "");
  AssertOneHeaderDirective("foo=", "foo", "");
  AssertOneHeaderDirective("foo===", "foo", "");
  AssertOneHeaderDirective("foo,", "foo", "");
  AssertOneHeaderDirective("foo,,,", "foo", "");
  AssertOneHeaderDirective("foo;", "foo", "");
  AssertOneHeaderDirective("foo;;;", "foo", "");
  AssertOneHeaderDirective("foo=bar", "foo", "bar");
  AssertOneHeaderDirective("foo=bar,foo=baz", "foo", "baz");
  AssertOneHeaderDirective("foo=\"bar, baz\"", "foo", "\"bar, baz\"");
  AssertOneHeaderDirective("foo=bar;foo=baz", "foo", "baz");
  AssertOneHeaderDirective("foo=\"bar; baz\"", "foo", "\"bar; baz\"");
}

TEST_F(HeaderDirectiveTest, MultipleHeaderDirectives) {
  AssertTwoHeaderDirectives("foo,bar", "foo", "", "bar", "");
  AssertTwoHeaderDirectives("foo, bar", "foo", "", "bar", "");
  AssertTwoHeaderDirectives("foo=, bar=", "foo", "", "bar", "");
  AssertTwoHeaderDirectives("foo=a, bar=b", "foo", "a", "bar", "b");
  AssertTwoHeaderDirectives("foo = a, bar= b", "foo", "a", "bar", "b");
  AssertTwoHeaderDirectives(
      "foo = \"bar baz \", bar= b", "foo", "\"bar baz \"", "bar", "b");

  AssertTwoHeaderDirectives("private, max-age=0", "private", "", "max-age", "0");
  AssertTwoHeaderDirectives(
      "text/html; charset=UTF8", "text/html", "", "charset", "UTF8");
}

TEST_F(HeaderDirectiveTest, BadHeaderDirectives) {
  AssertBadHeaderDirectives("=");
  AssertBadHeaderDirectives("====");
  AssertBadHeaderDirectives(",=");
  AssertBadHeaderDirectives("=,");
  AssertBadHeaderDirectives("====,");
  AssertBadHeaderDirectives(",====");
  AssertBadHeaderDirectives(",=,=,");
  AssertBadHeaderDirectives("=,=,=");
  AssertBadHeaderDirectives("  =,=,=  ");
  AssertBadHeaderDirectives("  =  ,  =  ,  =  ");
  AssertBadHeaderDirectives("=foo");
  AssertBadHeaderDirectives("foo,=");
  AssertBadHeaderDirectives(",=,foo=,=");
  AssertBadHeaderDirectives(" , foo = , =");
  AssertBadHeaderDirectives("foo=,=");
  AssertBadHeaderDirectives("foo bar");
  AssertBadHeaderDirectives("foo=bar baz");
  AssertBadHeaderDirectives("foo,bar baz");
  AssertBadHeaderDirectives("foo bar,baz");
  AssertBadHeaderDirectives("\"foo bar\"");
  AssertBadHeaderDirectives("foo \"foo bar\"");
  AssertBadHeaderDirectives("foo,\"foo bar\"");
  AssertBadHeaderDirectives("foo=bar, \"foo bar\"");
}

TEST_F(ResourceUtilTest, StaticResourceCacheControlNoCache) {
  // Base test: First specify a content type that's generally
  // cacheable.
  r_.AddResponseHeader("Content-Type", "image/png");
  EXPECT_FALSE(resource_util::HasExplicitNoCacheDirective(r_));
  EXPECT_TRUE(resource_util::IsLikelyStaticResource(r_));

  r_.AddResponseHeader("Cache-Control", "no-cache");
  EXPECT_TRUE(resource_util::HasExplicitNoCacheDirective(r_));
  EXPECT_FALSE(resource_util::IsLikelyStaticResource(r_));
}

TEST_F(ResourceUtilTest, StaticResourceCacheControlNoStore) {
  // Base test: First specify a content type that's generally
  // cacheable.
  r_.AddResponseHeader("Content-Type", "image/png");
  EXPECT_FALSE(resource_util::HasExplicitNoCacheDirective(r_));
  EXPECT_TRUE(resource_util::IsLikelyStaticResource(r_));

  r_.AddResponseHeader("Cache-Control", "no-store");
  EXPECT_TRUE(resource_util::HasExplicitNoCacheDirective(r_));
  EXPECT_FALSE(resource_util::IsLikelyStaticResource(r_));
}

TEST_F(ResourceUtilTest, StaticResourcePagmaNoCache) {
  // Base test: First specify a content type that's generally
  // cacheable.
  r_.AddResponseHeader("Content-Type", "image/png");
  EXPECT_FALSE(resource_util::HasExplicitNoCacheDirective(r_));
  EXPECT_TRUE(resource_util::IsLikelyStaticResource(r_));

  r_.AddResponseHeader("Pragma", "no-cache");
  EXPECT_TRUE(resource_util::HasExplicitNoCacheDirective(r_));
  EXPECT_FALSE(resource_util::IsLikelyStaticResource(r_));
}

TEST_F(ResourceUtilTest, StaticResourceVaryAll) {
  // Base test: First specify a content type that's generally
  // cacheable.
  r_.AddResponseHeader("Content-Type", "image/png");
  EXPECT_FALSE(resource_util::HasExplicitNoCacheDirective(r_));
  EXPECT_TRUE(resource_util::IsLikelyStaticResource(r_));

  r_.AddResponseHeader("Vary", "*");
  EXPECT_TRUE(resource_util::HasExplicitNoCacheDirective(r_));
  EXPECT_FALSE(resource_util::IsLikelyStaticResource(r_));
}

TEST_F(ResourceUtilTest, StaticResourceVaryContentEncoding) {
  // Base test: First specify a content type that's generally
  // cacheable.
  r_.AddResponseHeader("Content-Type", "image/png");
  EXPECT_FALSE(resource_util::HasExplicitNoCacheDirective(r_));
  EXPECT_TRUE(resource_util::IsLikelyStaticResource(r_));

  r_.AddResponseHeader("Vary", "Content-Encoding");
  EXPECT_FALSE(resource_util::HasExplicitNoCacheDirective(r_));
  EXPECT_TRUE(resource_util::IsLikelyStaticResource(r_));
}

TEST_F(ResourceUtilTest, QueryStringNotCacheable) {
  // First specify a content type that's known to be generally cacheable.
  r_.AddResponseHeader("Content-Type", "image/png");

  EXPECT_TRUE(resource_util::IsLikelyStaticResource(r_));

  // Adding a query string to the URL should make the resource
  // non-cacheable.
  r_.SetRequestUrl("http://www.example.com/hello?q=foo&a=b");
  EXPECT_FALSE(resource_util::IsLikelyStaticResource(r_));
}

TEST_F(ResourceUtilTest, StaticResourceNoContentType) {
  ASSERT_FALSE(resource_util::IsLikelyStaticResourceType(r_.GetResourceType()));
  ASSERT_FALSE(resource_util::IsLikelyStaticResource(r_));
}

TEST_F(ResourceUtilTest, StaticResourceHtml) {
  r_.AddResponseHeader("Content-Type", "text/html");
  ASSERT_FALSE(resource_util::IsLikelyStaticResourceType(r_.GetResourceType()));
  ASSERT_FALSE(resource_util::IsLikelyStaticResource(r_));
}

TEST_F(ResourceUtilTest, StaticResourceText) {
  r_.AddResponseHeader("Content-Type", "text/plain");
  ASSERT_FALSE(resource_util::IsLikelyStaticResourceType(r_.GetResourceType()));
  ASSERT_FALSE(resource_util::IsLikelyStaticResource(r_));
}

TEST_F(ResourceUtilTest, StaticResourceCss) {
  r_.AddResponseHeader("Content-Type", "text/css");
  ASSERT_TRUE(resource_util::IsLikelyStaticResourceType(r_.GetResourceType()));
  ASSERT_TRUE(resource_util::IsLikelyStaticResource(r_));
}

TEST_F(ResourceUtilTest, StaticResourceImage) {
  r_.AddResponseHeader("Content-Type", "image/png");
  ASSERT_TRUE(resource_util::IsLikelyStaticResourceType(r_.GetResourceType()));
  ASSERT_TRUE(resource_util::IsLikelyStaticResource(r_));
}

TEST_F(ResourceUtilTest, StaticResourceJavascript) {
  r_.AddResponseHeader("Content-Type", "application/x-javascript");
  ASSERT_TRUE(resource_util::IsLikelyStaticResourceType(r_.GetResourceType()));
  ASSERT_TRUE(resource_util::IsLikelyStaticResource(r_));
}

TEST_F(ResourceUtilTest, StaticResourceUnknownContentType) {
  r_.AddResponseHeader("Content-Type", "foo");
  ASSERT_FALSE(resource_util::IsLikelyStaticResourceType(r_.GetResourceType()));
  ASSERT_FALSE(resource_util::IsLikelyStaticResource(r_));
}

// Tests for status codes that are cacheable for all content types.
TEST_F(ResourceUtilTest, StaticResourceStatusCodes1) {
  // Base test: First specify a content type that's generally
  // non-cacheable.
  r_.AddResponseHeader("Content-Type", "text/html");
  EXPECT_FALSE(resource_util::IsLikelyStaticResource(r_));

  // A few codes are cacheable regardless of content type.
  //
  // TODO: do browsers automatically mark these a cacheable forever?
  // If so there's not much we can suggest to make them more
  // cacheable.
  r_.SetResponseStatusCode(300);
  EXPECT_TRUE(resource_util::IsCacheableResponseStatusCode(
      r_.GetResponseStatusCode()));
  EXPECT_TRUE(resource_util::IsLikelyStaticResource(r_));

  r_.SetResponseStatusCode(301);
  EXPECT_TRUE(resource_util::IsCacheableResponseStatusCode(
      r_.GetResponseStatusCode()));
  EXPECT_TRUE(resource_util::IsLikelyStaticResource(r_));

  r_.SetResponseStatusCode(410);
  EXPECT_TRUE(resource_util::IsCacheableResponseStatusCode(
      r_.GetResponseStatusCode()));
  EXPECT_TRUE(resource_util::IsLikelyStaticResource(r_));
}

// Tests for status codes that are cacheable for generally cacheable
// content types.
TEST_F(ResourceUtilTest, StaticResourceStatusCodesContentType) {
  // Base test: First specify a content type that's generally
  // cacheable.
  r_.AddResponseHeader("Content-Type", "image/png");
  EXPECT_TRUE(resource_util::IsLikelyStaticResource(r_));

  // Switching to a non-cacheable response code should cause the
  // result to change.
  r_.SetResponseStatusCode(100);
  EXPECT_FALSE(resource_util::IsCacheableResponseStatusCode(
      r_.GetResponseStatusCode()));
  EXPECT_FALSE(resource_util::IsLikelyStaticResource(r_));

  // The following codes should be cacheable.
  r_.SetResponseStatusCode(200);
  EXPECT_TRUE(resource_util::IsCacheableResponseStatusCode(
      r_.GetResponseStatusCode()));
  EXPECT_TRUE(resource_util::IsLikelyStaticResource(r_));

  r_.SetResponseStatusCode(203);
  EXPECT_TRUE(resource_util::IsCacheableResponseStatusCode(
      r_.GetResponseStatusCode()));
  EXPECT_TRUE(resource_util::IsLikelyStaticResource(r_));

  r_.SetResponseStatusCode(206);
  EXPECT_TRUE(resource_util::IsCacheableResponseStatusCode(
      r_.GetResponseStatusCode()));
  EXPECT_TRUE(resource_util::IsLikelyStaticResource(r_));

  r_.SetResponseStatusCode(300);
  EXPECT_TRUE(resource_util::IsCacheableResponseStatusCode(
      r_.GetResponseStatusCode()));
  EXPECT_TRUE(resource_util::IsLikelyStaticResource(r_));

  r_.SetResponseStatusCode(304);
  EXPECT_TRUE(resource_util::IsCacheableResponseStatusCode(
      r_.GetResponseStatusCode()));
  EXPECT_TRUE(resource_util::IsLikelyStaticResource(r_));
}

TEST_F(ResourceUtilTest, ParseTimeValuedHeader) {
  int64_t time = 0;
  EXPECT_TRUE(resource_util::ParseTimeValuedHeader(
      "Mon Mar 15 16:04:23 EDT 2010", &time));
  EXPECT_EQ(1268683463000LL, time);

  EXPECT_TRUE(resource_util::ParseTimeValuedHeader(
      "22-AUG-1993 10:59:12", &time));
  EXPECT_EQ(746031552000LL, time);

  // Not valid date strings.
  EXPECT_FALSE(resource_util::ParseTimeValuedHeader("0", &time));
  EXPECT_FALSE(resource_util::ParseTimeValuedHeader("", &time));
}

}  // namespace
