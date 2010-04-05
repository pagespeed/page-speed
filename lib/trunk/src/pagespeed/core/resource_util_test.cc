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

class StaticResourceTest : public ResourceUtilTest {};

TEST_F(StaticResourceTest, CacheControlNoCache) {
  // Base test: First specify a content type that's generally
  // cacheable.
  r_.AddResponseHeader("Content-Type", "image/png");
  EXPECT_FALSE(resource_util::HasExplicitNoCacheDirective(r_));
  EXPECT_TRUE(resource_util::IsLikelyStaticResource(r_));

  r_.AddResponseHeader("Cache-Control", "no-cache");
  EXPECT_TRUE(resource_util::HasExplicitNoCacheDirective(r_));
  EXPECT_FALSE(resource_util::IsLikelyStaticResource(r_));
}

TEST_F(StaticResourceTest, CacheControlNoStore) {
  // Base test: First specify a content type that's generally
  // cacheable.
  r_.AddResponseHeader("Content-Type", "image/png");
  EXPECT_FALSE(resource_util::HasExplicitNoCacheDirective(r_));
  EXPECT_TRUE(resource_util::IsLikelyStaticResource(r_));

  r_.AddResponseHeader("Cache-Control", "no-store");
  EXPECT_TRUE(resource_util::HasExplicitNoCacheDirective(r_));
  EXPECT_FALSE(resource_util::IsLikelyStaticResource(r_));
}

TEST_F(StaticResourceTest, MaxAgeZero) {
  // Base test: First specify a content type that's generally
  // cacheable.
  r_.AddResponseHeader("Content-Type", "image/png");
  EXPECT_FALSE(resource_util::HasExplicitNoCacheDirective(r_));
  EXPECT_TRUE(resource_util::IsLikelyStaticResource(r_));

  r_.AddResponseHeader("Cache-Control", "max-age=0");
  EXPECT_TRUE(resource_util::HasExplicitNoCacheDirective(r_));
  EXPECT_FALSE(resource_util::IsLikelyStaticResource(r_));
}

TEST_F(StaticResourceTest, ExpiresZero) {
  // Base test: First specify a content type that's generally
  // cacheable.
  r_.AddResponseHeader("Content-Type", "image/png");
  EXPECT_FALSE(resource_util::HasExplicitNoCacheDirective(r_));
  EXPECT_TRUE(resource_util::IsLikelyStaticResource(r_));

  r_.AddResponseHeader("Expires", "0");
  EXPECT_TRUE(resource_util::HasExplicitNoCacheDirective(r_));
  EXPECT_FALSE(resource_util::IsLikelyStaticResource(r_));
}

TEST_F(StaticResourceTest, PagmaNoCache) {
  // Base test: First specify a content type that's generally
  // cacheable.
  r_.AddResponseHeader("Content-Type", "image/png");
  EXPECT_FALSE(resource_util::HasExplicitNoCacheDirective(r_));
  EXPECT_TRUE(resource_util::IsLikelyStaticResource(r_));

  r_.AddResponseHeader("Pragma", "no-cache");
  EXPECT_TRUE(resource_util::HasExplicitNoCacheDirective(r_));
  EXPECT_FALSE(resource_util::IsLikelyStaticResource(r_));
}

TEST_F(StaticResourceTest, VaryAll) {
  // Base test: First specify a content type that's generally
  // cacheable.
  r_.AddResponseHeader("Content-Type", "image/png");
  EXPECT_FALSE(resource_util::HasExplicitNoCacheDirective(r_));
  EXPECT_TRUE(resource_util::IsLikelyStaticResource(r_));

  r_.AddResponseHeader("Vary", "*");
  EXPECT_TRUE(resource_util::HasExplicitNoCacheDirective(r_));
  EXPECT_FALSE(resource_util::IsLikelyStaticResource(r_));
}

TEST_F(StaticResourceTest, VaryContentEncoding) {
  // Base test: First specify a content type that's generally
  // cacheable.
  r_.AddResponseHeader("Content-Type", "image/png");
  EXPECT_FALSE(resource_util::HasExplicitNoCacheDirective(r_));
  EXPECT_TRUE(resource_util::IsLikelyStaticResource(r_));

  r_.AddResponseHeader("Vary", "Content-Encoding");
  EXPECT_FALSE(resource_util::HasExplicitNoCacheDirective(r_));
  EXPECT_TRUE(resource_util::IsLikelyStaticResource(r_));
}

TEST_F(StaticResourceTest, QueryStringNotCacheable) {
  // First specify a content type that's known to be generally cacheable.
  r_.AddResponseHeader("Content-Type", "image/png");

  EXPECT_TRUE(resource_util::IsLikelyStaticResource(r_));

  // Adding a query string to the URL should make the resource
  // non-cacheable.
  r_.SetRequestUrl("http://www.example.com/hello?q=foo&a=b");
  EXPECT_FALSE(resource_util::IsLikelyStaticResource(r_));
}

TEST_F(StaticResourceTest, NoContentType) {
  ASSERT_FALSE(resource_util::IsLikelyStaticResourceType(r_.GetResourceType()));
  ASSERT_FALSE(resource_util::IsLikelyStaticResource(r_));
}

TEST_F(StaticResourceTest, Html) {
  r_.AddResponseHeader("Content-Type", "text/html");
  ASSERT_FALSE(resource_util::IsLikelyStaticResourceType(r_.GetResourceType()));
  ASSERT_FALSE(resource_util::IsLikelyStaticResource(r_));
}

TEST_F(StaticResourceTest, Text) {
  r_.AddResponseHeader("Content-Type", "text/plain");
  ASSERT_FALSE(resource_util::IsLikelyStaticResourceType(r_.GetResourceType()));
  ASSERT_FALSE(resource_util::IsLikelyStaticResource(r_));
}

TEST_F(StaticResourceTest, Css) {
  r_.AddResponseHeader("Content-Type", "text/css");
  ASSERT_TRUE(resource_util::IsLikelyStaticResourceType(r_.GetResourceType()));
  ASSERT_TRUE(resource_util::IsLikelyStaticResource(r_));
}

TEST_F(StaticResourceTest, Image) {
  r_.AddResponseHeader("Content-Type", "image/png");
  ASSERT_TRUE(resource_util::IsLikelyStaticResourceType(r_.GetResourceType()));
  ASSERT_TRUE(resource_util::IsLikelyStaticResource(r_));
}

TEST_F(StaticResourceTest, Javascript) {
  r_.AddResponseHeader("Content-Type", "application/x-javascript");
  ASSERT_TRUE(resource_util::IsLikelyStaticResourceType(r_.GetResourceType()));
  ASSERT_TRUE(resource_util::IsLikelyStaticResource(r_));
}

TEST_F(StaticResourceTest, UnknownContentType) {
  r_.AddResponseHeader("Content-Type", "foo");
  ASSERT_FALSE(resource_util::IsLikelyStaticResourceType(r_.GetResourceType()));
  ASSERT_FALSE(resource_util::IsLikelyStaticResource(r_));
}

TEST_F(StaticResourceTest, AlwaysCacheableStatusCodes) {
  // Base test: First specify a content type that's generally
  // non-cacheable.
  r_.AddResponseHeader("Content-Type", "text/html");
  EXPECT_FALSE(resource_util::IsLikelyStaticResource(r_));

  // A few codes are cacheable regardless of content type.
  r_.SetResponseStatusCode(300);
  EXPECT_FALSE(resource_util::IsCacheableResource(r_));
  EXPECT_FALSE(resource_util::IsLikelyStaticResource(r_));

  r_.SetResponseStatusCode(301);
  EXPECT_FALSE(resource_util::IsCacheableResource(r_));
  EXPECT_FALSE(resource_util::IsLikelyStaticResource(r_));

  r_.SetResponseStatusCode(410);
  EXPECT_FALSE(resource_util::IsCacheableResource(r_));
  EXPECT_FALSE(resource_util::IsLikelyStaticResource(r_));
}

// Tests for status codes that are cacheable for generally cacheable
// content types.
TEST_F(StaticResourceTest, StatusCodesContentType) {
  // Base test: First specify a content type that's generally
  // cacheable.
  r_.AddResponseHeader("Content-Type", "image/png");
  EXPECT_TRUE(resource_util::IsLikelyStaticResource(r_));

  // Switching to a non-cacheable response code should cause the
  // result to change.
  r_.SetResponseStatusCode(100);
  EXPECT_FALSE(resource_util::IsCacheableResource(r_));
  EXPECT_FALSE(resource_util::IsLikelyStaticResource(r_));

  // The following codes should be cacheable.
  r_.SetResponseStatusCode(200);
  EXPECT_TRUE(resource_util::IsCacheableResource(r_));
  EXPECT_TRUE(resource_util::IsLikelyStaticResource(r_));

  r_.SetResponseStatusCode(203);
  EXPECT_TRUE(resource_util::IsCacheableResource(r_));
  EXPECT_TRUE(resource_util::IsLikelyStaticResource(r_));

  r_.SetResponseStatusCode(206);
  EXPECT_TRUE(resource_util::IsCacheableResource(r_));
  EXPECT_TRUE(resource_util::IsLikelyStaticResource(r_));

  r_.SetResponseStatusCode(300);
  EXPECT_FALSE(resource_util::IsCacheableResource(r_));
  EXPECT_FALSE(resource_util::IsLikelyStaticResource(r_));

  r_.SetResponseStatusCode(304);
  EXPECT_TRUE(resource_util::IsCacheableResource(r_));
  EXPECT_TRUE(resource_util::IsLikelyStaticResource(r_));
}

TEST_F(StaticResourceTest, Expired) {
  // Base test: First specify a content type that's generally
  // cacheable.
  r_.AddResponseHeader("Content-Type", "image/png");
  EXPECT_TRUE(resource_util::IsLikelyStaticResource(r_));

  // Add a header indicating that the resource is not fresh and verify
  // that it's no longer considered a static resource.
  r_.AddResponseHeader("Cache-Control", "max-age=0");
  EXPECT_FALSE(resource_util::IsLikelyStaticResource(r_));
}

TEST_F(ResourceUtilTest, ParseTimeValuedHeader) {
  int64 time = 0;
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

class GetFreshnessLifetimeTest : public ResourceUtilTest {
 protected:
  virtual void SetUp() {
    ResourceUtilTest::SetUp();
    freshness_lifetime_ = 0LL;
  }
  int64 freshness_lifetime_;
};

TEST_F(GetFreshnessLifetimeTest, NoHeaders) {
  EXPECT_FALSE(resource_util::GetFreshnessLifetimeMillis(r_,
                                                         &freshness_lifetime_));
}

TEST_F(GetFreshnessLifetimeTest, CacheControlNoMaxAge) {
  r_.AddResponseHeader("Cache-Control", "foo=bar");
  EXPECT_FALSE(resource_util::GetFreshnessLifetimeMillis(r_,
                                                         &freshness_lifetime_));
}

TEST_F(GetFreshnessLifetimeTest, EmptyMaxAge) {
  r_.AddResponseHeader("Cache-Control", "max-age=");
  EXPECT_FALSE(resource_util::GetFreshnessLifetimeMillis(r_,
                                                         &freshness_lifetime_));
}

TEST_F(GetFreshnessLifetimeTest, MaxAge1) {
  r_.AddResponseHeader("Cache-Control", "max-age=0");
  EXPECT_TRUE(resource_util::GetFreshnessLifetimeMillis(r_,
                                                        &freshness_lifetime_));
  EXPECT_EQ(0LL, freshness_lifetime_);
}

TEST_F(GetFreshnessLifetimeTest, MaxAge2) {
  r_.AddResponseHeader("Cache-Control", "max-age=10");
  EXPECT_TRUE(resource_util::GetFreshnessLifetimeMillis(r_,
                                                        &freshness_lifetime_));
  EXPECT_EQ(10000LL, freshness_lifetime_);
}

TEST_F(GetFreshnessLifetimeTest, MaxAgeIgnoredIfExplicitNoCacheDirective) {
  r_.AddResponseHeader("Cache-Control", "max-age=10, no-cache");
  EXPECT_TRUE(resource_util::GetFreshnessLifetimeMillis(r_,
                                                        &freshness_lifetime_));
  EXPECT_EQ(0LL, freshness_lifetime_);
}

TEST_F(GetFreshnessLifetimeTest, BadMaxAge) {
  r_.AddResponseHeader("Cache-Control", "max-age=foo");
  EXPECT_FALSE(resource_util::GetFreshnessLifetimeMillis(r_,
                                                         &freshness_lifetime_));
}

TEST_F(GetFreshnessLifetimeTest, BadExpires) {
  r_.AddResponseHeader("Expires", "0");
  r_.AddResponseHeader("Date", "Tue, 16 Mar 2010 16:08:25 EDT");
  EXPECT_TRUE(resource_util::GetFreshnessLifetimeMillis(r_,
                                                        &freshness_lifetime_));
  EXPECT_EQ(0LL, freshness_lifetime_);
}

TEST_F(GetFreshnessLifetimeTest, FutureExpiresWithDate) {
  r_.AddResponseHeader("Expires", "Wed, 17 Mar 2010 16:08:25 EDT");
  r_.AddResponseHeader("Date", "Tue, 16 Mar 2010 16:08:25 EDT");
  EXPECT_TRUE(resource_util::GetFreshnessLifetimeMillis(r_,
                                                        &freshness_lifetime_));
  EXPECT_EQ(86400000LL, freshness_lifetime_);
}

TEST_F(GetFreshnessLifetimeTest, PastExpiresWithDate) {
  r_.AddResponseHeader("Expires", "Tue, 16 Mar 2010 16:08:25 EDT");
  r_.AddResponseHeader("Date", "Wed, 17 Mar 2010 16:08:25 EDT");
  EXPECT_TRUE(resource_util::GetFreshnessLifetimeMillis(r_,
                                                        &freshness_lifetime_));
  EXPECT_EQ(0LL, freshness_lifetime_);
}

TEST_F(GetFreshnessLifetimeTest, ExpiresNoDateNoResponseTime) {
  r_.AddResponseHeader("Expires", "Wed, 17 Mar 2010 16:08:25 EDT");
  EXPECT_FALSE(resource_util::GetFreshnessLifetimeMillis(r_,
                                                         &freshness_lifetime_));
}

TEST_F(GetFreshnessLifetimeTest, PreferMaxAgeToExpires) {
  r_.AddResponseHeader("Expires", "Wed, 17 Mar 2010 16:08:25 EDT");
  r_.AddResponseHeader("Date", "Tue, 16 Mar 2010 16:08:25 EDT");
  EXPECT_TRUE(resource_util::GetFreshnessLifetimeMillis(r_,
                                                        &freshness_lifetime_));
  EXPECT_EQ(86400000LL, freshness_lifetime_);

  // Now add a max-age header, and verify that it's preferred to the
  // Expires value.
  r_.AddResponseHeader("Cache-Control", "max-age=100");
  EXPECT_TRUE(resource_util::GetFreshnessLifetimeMillis(r_,
                                                        &freshness_lifetime_));
  EXPECT_EQ(100000LL, freshness_lifetime_);
}

}  // namespace
