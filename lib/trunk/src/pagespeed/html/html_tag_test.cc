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

#include "pagespeed/html/html_tag.h"
#include "testing/gtest/include/gtest/gtest.h"

using pagespeed::html::HtmlTag;

namespace {

TEST(HtmlTagTest, Basic) {
  const std::string input("<foo bar baz=quux blah=''>");
  const char* begin = input.data();
  const char* end = begin + input.size();
  HtmlTag tag;
  ASSERT_EQ(end, tag.ReadTag(begin, end));

  ASSERT_EQ("foo", tag.tagname());
  ASSERT_EQ("foo", tag.GetBaseTagName());
  ASSERT_FALSE(tag.IsEmptyElement());
  ASSERT_FALSE(tag.IsEndTag());
  ASSERT_FALSE(tag.HasAttr("foo"));
  ASSERT_TRUE(tag.HasAttr("bar"));
  ASSERT_FALSE(tag.HasAttrValue("bar"));
  ASSERT_TRUE(tag.HasAttr("baz"));
  ASSERT_TRUE(tag.HasAttrValue("baz"));
  ASSERT_EQ("quux", tag.GetAttrValue("baz"));
  ASSERT_FALSE(tag.HasAttr("quux"));
  ASSERT_TRUE(tag.HasAttr("blah"));
  ASSERT_TRUE(tag.HasAttrValue("blah"));
  ASSERT_EQ("", tag.GetAttrValue("blah"));
  ASSERT_EQ("<foo bar baz=quux blah=\"\">", tag.ToString());
}

TEST(HtmlTagTest, EndTag) {
  const std::string input("</foo>");
  const char* begin = input.data();
  const char* end = begin + input.size();
  HtmlTag tag;
  ASSERT_EQ(end, tag.ReadTag(begin, end));

  ASSERT_EQ("/foo", tag.tagname());
  ASSERT_EQ("foo", tag.GetBaseTagName());
  ASSERT_FALSE(tag.IsEmptyElement());
  ASSERT_TRUE(tag.IsEndTag());
  ASSERT_FALSE(tag.HasAttr("foo"));
  ASSERT_FALSE(tag.HasAttr("bar"));
  ASSERT_EQ(input, tag.ToString());
}

TEST(HtmlTagTest, SelfClosingTag) {
  const std::string input("<foobar  foo=\"bar\" />");
  const char* begin = input.data();
  const char* end = begin + input.size();
  HtmlTag tag;
  ASSERT_EQ(end, tag.ReadTag(begin, end));

  ASSERT_EQ("foobar", tag.tagname());
  ASSERT_TRUE(tag.IsEmptyElement());
  ASSERT_TRUE(tag.IsEndTag());
  ASSERT_TRUE(tag.HasAttr("foo"));
  ASSERT_TRUE(tag.HasAttrValue("foo"));
  ASSERT_EQ("bar", tag.GetAttrValue("foo"));
  ASSERT_EQ("<foobar foo=bar />", tag.ToString());
}

TEST(HtmlTagTest, RepeatedAttrWithoutValue) {
  const std::string input("<foo bar bar>");
  const char* begin = input.data();
  const char* end = begin + input.size();
  HtmlTag tag;
  ASSERT_EQ(end, tag.ReadTag(begin, end));

  ASSERT_EQ("foo", tag.tagname());
  ASSERT_TRUE(tag.HasAttr("bar"));
  ASSERT_FALSE(tag.HasAttrValue("bar"));
  ASSERT_EQ("<foo bar>", tag.ToString());
}

TEST(HtmlTagTest, RepeatedAttrWithValue) {
  // Unfortunately, I couldn't find anything specified in an RFC about how to
  // handle repeated attributes like this, but Firefox and Chrome both seem to
  // ignore all but the first value given for the attribute, so that's what
  // HtmlTag does too.  (mdsteele)
  const std::string input("<foo bar=baz bar=quux>");
  const char* begin = input.data();
  const char* end = begin + input.size();
  HtmlTag tag;
  ASSERT_EQ(end, tag.ReadTag(begin, end));

  ASSERT_EQ("foo", tag.tagname());
  ASSERT_TRUE(tag.HasAttr("bar"));
  ASSERT_TRUE(tag.HasAttrValue("bar"));
  ASSERT_EQ("baz", tag.GetAttrValue("bar"));
  ASSERT_EQ("<foo bar=baz>", tag.ToString());
}

TEST(HtmlTagTest, Comment) {
  const std::string input("<!-- foo -->");
  const char* begin = input.data();
  const char* end = begin + input.size();
  HtmlTag tag;
  ASSERT_EQ(end, tag.ReadTag(begin, end));

  ASSERT_EQ("!--", tag.tagname());
  ASSERT_FALSE(tag.IsEmptyElement());
  ASSERT_FALSE(tag.IsEndTag());
  ASSERT_FALSE(tag.HasAttr("foo"));
  ASSERT_EQ("<!-->", tag.ToString());
}

TEST(HtmlTagTest, Lowercasify) {
  const std::string input("<Foo BAR bAz=quUx>");
  const char* begin = input.data();
  const char* end = begin + input.size();
  HtmlTag tag;
  ASSERT_EQ(end, tag.ReadTag(begin, end));

  ASSERT_EQ("foo", tag.tagname());
  ASSERT_EQ("foo", tag.GetBaseTagName());
  ASSERT_TRUE(tag.HasAttr("bar"));
  ASSERT_FALSE(tag.HasAttr("BAR"));
  ASSERT_FALSE(tag.HasAttrValue("bar"));
  ASSERT_TRUE(tag.HasAttr("baz"));
  ASSERT_FALSE(tag.HasAttr("bAz"));
  ASSERT_TRUE(tag.HasAttrValue("baz"));
  ASSERT_EQ("quUx", tag.GetAttrValue("baz"));
  ASSERT_EQ("<foo bar baz=quUx>", tag.ToString());
}

TEST(HtmlTagTest, ReuseTagObject) {
  HtmlTag tag;

  const std::string input1("<foo bar baz=quux>");
  const char* begin1 = input1.data();
  const char* end1 = begin1 + input1.size();
  ASSERT_EQ(end1, tag.ReadTag(begin1, end1));

  ASSERT_EQ("foo", tag.tagname());
  ASSERT_EQ("foo", tag.GetBaseTagName());
  ASSERT_FALSE(tag.IsEmptyElement());
  ASSERT_FALSE(tag.IsEndTag());
  ASSERT_TRUE(tag.HasAttr("bar"));
  ASSERT_FALSE(tag.HasAttrValue("bar"));
  ASSERT_TRUE(tag.HasAttr("baz"));
  ASSERT_TRUE(tag.HasAttrValue("baz"));
  ASSERT_EQ("quux", tag.GetAttrValue("baz"));
  ASSERT_EQ(input1, tag.ToString());

  const std::string input2("</spam eggs=bacon>");
  const char* begin2 = input2.data();
  const char* end2 = begin2 + input2.size();
  ASSERT_EQ(end2, tag.ReadTag(begin2, end2));

  ASSERT_EQ("/spam", tag.tagname());
  ASSERT_EQ("spam", tag.GetBaseTagName());
  ASSERT_FALSE(tag.IsEmptyElement());
  ASSERT_TRUE(tag.IsEndTag());
  ASSERT_FALSE(tag.HasAttr("bar"));
  ASSERT_FALSE(tag.HasAttrValue("bar"));
  ASSERT_FALSE(tag.HasAttr("baz"));
  ASSERT_FALSE(tag.HasAttrValue("baz"));
  ASSERT_TRUE(tag.HasAttr("eggs"));
  ASSERT_TRUE(tag.HasAttrValue("eggs"));
  ASSERT_EQ("bacon", tag.GetAttrValue("eggs"));
  ASSERT_EQ(input2, tag.ToString());
}

TEST(HtmlTagTest, MinimalTags) {
  HtmlTag tag;

  const std::string input1("<x>");
  const char* begin1 = input1.data();
  const char* end1 = begin1 + input1.size();
  ASSERT_EQ(end1, tag.ReadTag(begin1, end1));
  ASSERT_EQ("x", tag.tagname());
  ASSERT_EQ("x", tag.GetBaseTagName());
  ASSERT_FALSE(tag.IsEndTag());
  ASSERT_EQ(input1, tag.ToString());

  const std::string input2("</>");
  const char* begin2 = input2.data();
  const char* end2 = begin2 + input2.size();
  ASSERT_EQ(end2, tag.ReadTag(begin2, end2));
  ASSERT_EQ("/", tag.tagname());
  ASSERT_EQ("", tag.GetBaseTagName());
  ASSERT_TRUE(tag.IsEndTag());
  ASSERT_EQ(input2, tag.ToString());

  const std::string input3("<>");
  const char* begin3 = input3.data();
  const char* end3 = begin3 + input3.size();
  ASSERT_EQ(NULL, tag.ReadTag(begin3, end3));
}

TEST(HtmlTagTest, ModifyTag) {
  const std::string input("<foo>");
  const char* begin = input.data();
  const char* end = begin + input.size();
  HtmlTag tag;
  ASSERT_EQ(end, tag.ReadTag(begin, end));

  ASSERT_FALSE(tag.HasAttr("bar"));
  ASSERT_FALSE(tag.HasAttrValue("bar"));
  ASSERT_EQ("<foo>", tag.ToString());

  tag.AddAttr("bar");
  ASSERT_TRUE(tag.HasAttr("bar"));
  ASSERT_FALSE(tag.HasAttrValue("bar"));
  ASSERT_EQ("<foo bar>", tag.ToString());

  tag.SetAttrValue("bar", "quux");
  ASSERT_TRUE(tag.HasAttr("bar"));
  ASSERT_TRUE(tag.HasAttrValue("bar"));
  ASSERT_EQ("quux", tag.GetAttrValue("bar"));
  ASSERT_EQ("<foo bar=quux>", tag.ToString());

  tag.ClearAttrValue("bar");
  ASSERT_TRUE(tag.HasAttr("bar"));
  ASSERT_FALSE(tag.HasAttrValue("bar"));
  ASSERT_EQ("<foo bar>", tag.ToString());

  tag.ClearAttr("bar");
  ASSERT_FALSE(tag.HasAttr("bar"));
  ASSERT_FALSE(tag.HasAttrValue("bar"));
  ASSERT_EQ("<foo>", tag.ToString());
}

TEST(HtmlTagTest, TwoTags) {
  const std::string input("<foo bar=\"baz\"></foo quux>");
  const char* begin = input.data();
  const char* end = begin + input.size();
  HtmlTag tag;
  ASSERT_EQ(end - strlen("</foo quux>"), tag.ReadTag(begin, end));

  ASSERT_TRUE(tag.HasAttr("bar"));
  ASSERT_TRUE(tag.HasAttrValue("bar"));
  ASSERT_EQ("baz", tag.GetAttrValue("bar"));
  ASSERT_FALSE(tag.HasAttr("quux"));
  ASSERT_EQ("<foo bar=baz>", tag.ToString());
}

TEST(HtmlTagTest, ReadNextTag) {
  const std::string input("blah blah <foo bar=baz><quux>");
  const char* begin = input.data();
  const char* end = begin + input.size();
  HtmlTag tag;
  ASSERT_EQ(end - strlen("<quux>"), tag.ReadNextTag(begin, end));

  ASSERT_EQ("foo", tag.tagname());
  ASSERT_FALSE(tag.IsEmptyElement());
  ASSERT_FALSE(tag.IsEndTag());
  ASSERT_TRUE(tag.HasAttr("bar"));
  ASSERT_TRUE(tag.HasAttrValue("bar"));
  ASSERT_EQ("baz", tag.GetAttrValue("bar"));
  ASSERT_EQ("<foo bar=baz>", tag.ToString());
}

TEST(HtmlTagTest, ReadNextTagAfterInvalidTag) {
  const std::string input("blah < quux blah <foo bar=baz>");
  const char* begin = input.data();
  const char* end = begin + input.size();
  HtmlTag tag;
  ASSERT_EQ(end, tag.ReadNextTag(begin, end));

  ASSERT_EQ("foo", tag.tagname());
  ASSERT_FALSE(tag.IsEmptyElement());
  ASSERT_FALSE(tag.IsEndTag());
  ASSERT_FALSE(tag.HasAttr("blah"));
  ASSERT_TRUE(tag.HasAttr("bar"));
  ASSERT_TRUE(tag.HasAttrValue("bar"));
  ASSERT_EQ("baz", tag.GetAttrValue("bar"));
  ASSERT_EQ("<foo bar=baz>", tag.ToString());
}

TEST(HtmlTagTest, ReadClosingForeignTag) {
  const std::string input("<script>document.write('</foo>')</script>");
  const char* begin = input.data();
  const char* end = begin + input.size();
  HtmlTag tag;

  const char* mid = tag.ReadTag(begin, end);
  ASSERT_EQ(begin + strlen("<script>"), mid);
  ASSERT_EQ("script", tag.tagname());
  ASSERT_FALSE(tag.IsEmptyElement());
  ASSERT_FALSE(tag.IsEndTag());
  ASSERT_EQ("<script>", tag.ToString());

  ASSERT_EQ(end, tag.ReadClosingForeignTag(mid, end));
  ASSERT_EQ("/script", tag.tagname());
  ASSERT_FALSE(tag.IsEmptyElement());
  ASSERT_TRUE(tag.IsEndTag());
  ASSERT_EQ("</script>", tag.ToString());
}

TEST(HtmlTagTest, TagNotAtStart) {
  const std::string input(" <foo bar=baz>");
  const char* begin = input.data();
  const char* end = begin + input.size();
  HtmlTag tag;
  ASSERT_EQ(NULL, tag.ReadTag(begin, end));
}

TEST(HtmlTagTest, UnfinishedTag) {
  const std::string input("<foo bar=baz ");
  const char* begin = input.data();
  const char* end = begin + input.size();
  HtmlTag tag;
  ASSERT_EQ(NULL, tag.ReadTag(begin, end));
}

TEST(HtmlTagTest, UnfinishedAttr) {
  const std::string input("<foo bar=\"baz");
  const char* begin = input.data();
  const char* end = begin + input.size();
  HtmlTag tag;
  ASSERT_EQ(NULL, tag.ReadTag(begin, end));
}

}  // namespace
