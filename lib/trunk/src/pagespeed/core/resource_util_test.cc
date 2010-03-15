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

#include "pagespeed/core/resource_util.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

class HeaderDirectiveTest : public testing::Test {
 protected:
  void AssertBadHeaderDirectives(const char *header) {
    m_.clear();
    ASSERT_FALSE(
        pagespeed::resource_util::GetHeaderDirectives(header, &m_));
    ASSERT_TRUE(m_.empty());
  }

  void AssertEmptyHeaderDirectives(const char *header) {
    m_.clear();
    ASSERT_TRUE(
        pagespeed::resource_util::GetHeaderDirectives(header, &m_));
    ASSERT_TRUE(m_.empty());
  }

  void AssertOneHeaderDirective(const char *header,
                          const char *key,
                          const char *value) {
    m_.clear();
    ASSERT_TRUE(
        pagespeed::resource_util::GetHeaderDirectives(header, &m_));
    ASSERT_EQ(1, m_.size());
    pagespeed::resource_util::DirectiveMap::const_iterator it = m_.find(key);
    ASSERT_TRUE(m_.end() != it);
    ASSERT_STREQ(value, it->second.c_str());
  }

  void AssertTwoHeaderDirectives(const char *header,
                           const char *key1,
                           const char *value1,
                           const char *key2,
                           const char *value2) {
    m_.clear();
    ASSERT_TRUE(
        pagespeed::resource_util::GetHeaderDirectives(header, &m_));
    ASSERT_EQ(2, m_.size());
    pagespeed::resource_util::DirectiveMap::const_iterator it = m_.find(key1);
    ASSERT_TRUE(m_.end() != it);
    ASSERT_STREQ(value1, it->second.c_str());
    it = m_.find(key2);
    ASSERT_TRUE(m_.end() != it);
    ASSERT_STREQ(value2, it->second.c_str());
  }

  pagespeed::resource_util::DirectiveMap m_;
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

  AssertTwoHeaderDirectives(
      "private, max-age=0", "private", "", "max-age", "0");
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

}  // namespace
