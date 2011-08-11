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

#include <set>

#include "pagespeed/core/resource.h"
#include "pagespeed/css/external_resource_finder.h"
#include "pagespeed/testing/pagespeed_test.h"

namespace {

using pagespeed::css::ExternalResourceFinder;

static const char* kCssUrl = "http://www.example.com/foo.css";
static const char* kImportUrl1 = "http://www.example.com/import1.css";
static const char* kImportUrl2 = "http://www.example.com/import2.css";

static const char* kNoImportBody =
    "body {\n"
    "color: purple;\n"
    "background-color: #d8da3d }";

static const char* kBasicImportBody =
    "@import \" http://www.example.com/import1.css \"";

static const char* kTwoBasicImportsBody =
    "@import url(\"http://www.example.com/import1.css\")\n"
    "@import url(\"http://www.example.com/import2.css\")";

static const char* kTwoRelativeImportsBody =
    "@import url(\" /import1.css \")\n"
    "@import url( import2.css )";

static const char* kOneImportBody =
    "/* comment\n"
    "   spans\n"
    "   multiple\n"
    "   lines\n"
    "*/ /*another comment*/ "
    "@iMpOrT url(\"http://www.example.com/import1.css\")";

static const char* kImportInCommentBody =
    "/* comment\n"
    "   spans\n"
    "   multiple\n"
    "   lines\n"
    "@iMpOrT url(\"http://www.example.com/import1.css\")*/";

static const char* kUnterminatedCommentBody =
    "/* comment\n"
    "   spans\n"
    "   multiple\n"
    "   lines\n"
    "@iMpOrT url(\"http://www.example.com/import1.css\");\n"
    "body {\n"
    "color: purple;\n"
    "background-color: #d8da3d }";

static const char* kBadImportUrlBody = "@import \"http://!@#$%^&*()/\"";

TEST(RemoveCssCommentsTest, EmptyBody) {
  std::string css;
  ExternalResourceFinder::RemoveComments("", &css);
  ASSERT_TRUE(css.empty());
}

TEST(RemoveCssCommentsTest, NoComments) {
  const char* kNoComments = "here is some text that does not contain comments";
  std::string css;
  ExternalResourceFinder::RemoveComments(kNoComments, &css);
  ASSERT_EQ(css, kNoComments);
}

TEST(RemoveCssCommentsTest, EmptyComment) {
  const char* kComment = "/**/";
  std::string css;
  ExternalResourceFinder::RemoveComments(kComment, &css);
  ASSERT_TRUE(css.empty());
}

TEST(RemoveCssCommentsTest, EmptyComments) {
  const char* kComment = "/**//**//**//**/";
  std::string css;
  ExternalResourceFinder::RemoveComments(kComment, &css);
  ASSERT_TRUE(css.empty());
}

TEST(RemoveCssCommentsTest, SimpleComment) {
  const char* kComment = "/* here is a comment*/";
  std::string css;
  ExternalResourceFinder::RemoveComments(kComment, &css);
  ASSERT_TRUE(css.empty());
}

TEST(RemoveCssCommentsTest, CommentAtBeginning) {
  const char* kComment = "/* here is a comment*/ content";
  std::string css;
  ExternalResourceFinder::RemoveComments(kComment, &css);
  ASSERT_EQ(" content", css);
}

TEST(RemoveCssCommentsTest, CommentAtEnd) {
  const char* kComment = "content /* here is a comment*/";
  std::string css;
  ExternalResourceFinder::RemoveComments(kComment, &css);
  ASSERT_EQ("content ", css);
}

TEST(RemoveCssCommentsTest, CommentAtBothEnds) {
  const char* kComment = "/* comment*/ content /* here is a comment*/";
  std::string css;
  ExternalResourceFinder::RemoveComments(kComment, &css);
  ASSERT_EQ(" content ", css);
}

TEST(RemoveCssCommentsTest, CommentInMiddle) {
  const char* kComment = "content /* comment*/ content";
  std::string css;
  ExternalResourceFinder::RemoveComments(kComment, &css);
  ASSERT_EQ("content  content", css);
}

TEST(RemoveCssCommentsTest, MultiLineComment) {
  const char* kComment = "/*here\nis\na\ncomment*/";
  std::string css;
  ExternalResourceFinder::RemoveComments(kComment, &css);
  ASSERT_TRUE(css.empty());
}

TEST(RemoveCssCommentsTest, MultipleComments) {
  const char* kComment = "/* here is a comment*//*here is another*/";
  std::string css;
  ExternalResourceFinder::RemoveComments(kComment, &css);
  ASSERT_TRUE(css.empty());
}

TEST(RemoveCssCommentsTest, MultipleCommentsContentBetween) {
  const char* kComment =
      "here /* here is a comment*/ is /*here is another*/ content";
  std::string css;
  ExternalResourceFinder::RemoveComments(kComment, &css);
  ASSERT_EQ(css, "here  is  content");
}

TEST(RemoveCssCommentsTest, MultipleMultiLineCommentsContentBetween) {
  const char* kComment =
      "here\n /*\nhere\nis\na\ncomment*/ is /*here\nis\nanother*/ \ncontent";
  std::string css;
  ExternalResourceFinder::RemoveComments(kComment, &css);
  ASSERT_EQ(css, "here\n  is  \ncontent");
}

TEST(RemoveCssCommentsTest, UnterminatedComment) {
  const char* kComment = "/*an unterminated comment";
  std::string css;
  ExternalResourceFinder::RemoveComments(kComment, &css);
  ASSERT_TRUE(css.empty());
}

TEST(RemoveCssCommentsTest, UnterminatedComment2) {
  const char* kComment = "here  is  content/*an unterminated comment";
  std::string css;
  ExternalResourceFinder::RemoveComments(kComment, &css);
  ASSERT_EQ(css, "here  is  content");
}

TEST(RemoveCssCommentsTest, UnterminatedComment3) {
  const char* kComment =
      "here/* */  is/* comment*/  content/*an unterminated comment";
  std::string css;
  ExternalResourceFinder::RemoveComments(kComment, &css);
  ASSERT_EQ(css, "here  is  content");
}

// Comments aren't allowed to be nested. Verify that we handle nested
// comments correctly. See
// http://www.w3.org/TR/CSS21/syndata.html#comments for more.
TEST(RemoveCssCommentsTest, NestedComment) {
  const char* kComment =
      "here  is  content /* here is /* a nested */ comment */";
  std::string css;
  ExternalResourceFinder::RemoveComments(kComment, &css);
  ASSERT_EQ(css, "here  is  content  comment */");
}

TEST(IsCssImportLineTest, String) {
  std::string url;
  ASSERT_FALSE(ExternalResourceFinder::IsCssImportLine("", &url));
  ASSERT_FALSE(ExternalResourceFinder::IsCssImportLine("foo {};", &url));
  ASSERT_FALSE(ExternalResourceFinder::IsCssImportLine("@iMpOrT", &url));
  ASSERT_FALSE(ExternalResourceFinder::IsCssImportLine("@iMpOrT", &url));
  ASSERT_FALSE(ExternalResourceFinder::IsCssImportLine("@iMpOrT \"", &url));
  ASSERT_FALSE(ExternalResourceFinder::IsCssImportLine("@iMpOrT '", &url));
  ASSERT_FALSE(ExternalResourceFinder::IsCssImportLine("@iMpOrT \"\"", &url));
  ASSERT_FALSE(ExternalResourceFinder::IsCssImportLine("@iMpOrT ''", &url));
  ASSERT_FALSE(ExternalResourceFinder::IsCssImportLine("@iMpOrT '\"", &url));
  ASSERT_FALSE(ExternalResourceFinder::IsCssImportLine("@iMpOrT \"'", &url));

  // Should not match if end quote is missing.
  ASSERT_FALSE(ExternalResourceFinder::IsCssImportLine(
      "@iMpOrT 'http://www.example.com/foo.css", &url));

  // Mismatched quotes for URL should not match.
  ASSERT_FALSE(ExternalResourceFinder::IsCssImportLine(
      "@iMpOrT 'http://www.example.com/foo.css\"", &url));

  // Test with single quotes.
  ASSERT_TRUE(ExternalResourceFinder::IsCssImportLine(
      "@iMpOrT 'http://www.example.com/foo.css'", &url));
  ASSERT_EQ("http://www.example.com/foo.css", url);

  // Test with double quotes.
  ASSERT_TRUE(ExternalResourceFinder::IsCssImportLine(
      "@iMpOrT \"http://www.example.com/foo.css\"", &url));
  ASSERT_EQ("http://www.example.com/foo.css", url);

  // Relative URL
  ASSERT_TRUE(ExternalResourceFinder::IsCssImportLine("@iMpOrT 'foo.css'", &url));
  ASSERT_EQ("foo.css", url);

  // Relative URL
  ASSERT_TRUE(ExternalResourceFinder::IsCssImportLine("@iMpOrT 'foo.css'", &url));
  ASSERT_EQ("foo.css", url);

  // No space
  ASSERT_TRUE(ExternalResourceFinder::IsCssImportLine("@iMpOrT'foo.css'", &url));
  ASSERT_EQ("foo.css", url);

  // Many spaces
  ASSERT_TRUE(ExternalResourceFinder::IsCssImportLine("@iMpOrT   'foo.css'", &url));
  ASSERT_EQ("foo.css", url);

  // Whitespace at ends of URL spaces (we do not trim).
  ASSERT_TRUE(ExternalResourceFinder::IsCssImportLine("@iMpOrT   ' foo.css '", &url));
  ASSERT_EQ(" foo.css ", url);
}

TEST(IsCssImportLineTest, Url) {
  std::string url;

  // No URL.
  ASSERT_FALSE(ExternalResourceFinder::IsCssImportLine("@iMpOrT UrL(", &url));
  ASSERT_FALSE(ExternalResourceFinder::IsCssImportLine("@iMpOrT UrL('", &url));
  ASSERT_FALSE(ExternalResourceFinder::IsCssImportLine("@iMpOrT UrL(''", &url));
  ASSERT_FALSE(ExternalResourceFinder::IsCssImportLine("@iMpOrT UrL('\"", &url));
  ASSERT_FALSE(ExternalResourceFinder::IsCssImportLine("@iMpOrT UrL()", &url));
  ASSERT_FALSE(ExternalResourceFinder::IsCssImportLine("@iMpOrT UrL(')", &url));
  ASSERT_FALSE(ExternalResourceFinder::IsCssImportLine("@iMpOrT UrL(\")", &url));
  ASSERT_FALSE(ExternalResourceFinder::IsCssImportLine("@iMpOrT UrL('')", &url));
  ASSERT_FALSE(ExternalResourceFinder::IsCssImportLine("@iMpOrT UrL(\"\")", &url));
  ASSERT_FALSE(ExternalResourceFinder::IsCssImportLine("@iMpOrT UrL('\")", &url));
  ASSERT_FALSE(ExternalResourceFinder::IsCssImportLine("@iMpOrT UrL(\"')", &url));

  // No space, non-terminated parenthesis.
  ASSERT_FALSE(ExternalResourceFinder::IsCssImportLine("@iMpOrTUrL('foo.css'", &url));

  // One space, non-terminated parenthesis.
  ASSERT_FALSE(ExternalResourceFinder::IsCssImportLine("@iMpOrT UrL('foo.css'", &url));

  // Multi spaces, non-terminated parenthesis.
  ASSERT_FALSE(ExternalResourceFinder::IsCssImportLine("@iMpOrT  UrL('foo.css'", &url));

  // One space, non-terminated parenthesis, no quotes.
  ASSERT_FALSE(ExternalResourceFinder::IsCssImportLine("@iMpOrT UrL(foo.css", &url));

  // Mismatched quotes for URL should not match.
  ASSERT_FALSE(ExternalResourceFinder::IsCssImportLine(
      "@iMpOrT uRl('http://www.example.com/foo.css\")", &url));

  // No space.
  ASSERT_TRUE(ExternalResourceFinder::IsCssImportLine("@iMpOrTUrL('foo.css')", &url));
  ASSERT_EQ("foo.css", url);

  // One space.
  ASSERT_TRUE(ExternalResourceFinder::IsCssImportLine("@iMpOrT UrL('foo.css')", &url));
  ASSERT_EQ("foo.css", url);

  // Multi spaces.
  ASSERT_TRUE(ExternalResourceFinder::IsCssImportLine("@iMpOrT  UrL('foo.css')", &url));
  ASSERT_EQ("foo.css", url);

  // Extra spaces.
  ASSERT_TRUE(
      ExternalResourceFinder::IsCssImportLine("@iMpOrT  UrL(' foo.css ')", &url));
  ASSERT_EQ(" foo.css ", url);

  // No quotes.
  ASSERT_TRUE(ExternalResourceFinder::IsCssImportLine("@iMpOrT UrL(foo.css)", &url));
  ASSERT_EQ("foo.css", url);

  // Extra spaces.
  ASSERT_TRUE(ExternalResourceFinder::IsCssImportLine("@iMpOrT UrL( foo.css )", &url));
  ASSERT_EQ("foo.css", url);

  // Short
  ASSERT_TRUE(ExternalResourceFinder::IsCssImportLine("@iMpOrT UrL(a)", &url));
  ASSERT_EQ("a", url);
}

class ExternalResourceFinderTest : public pagespeed_testing::PagespeedTest {
};

TEST_F(ExternalResourceFinderTest, EmptyBody) {
  pagespeed::Resource* r = NewCssResource(kCssUrl);

  std::set<std::string> urls;
  ExternalResourceFinder f;
  f.FindExternalResources(*r, &urls);
  ASSERT_TRUE(urls.empty());
}

TEST_F(ExternalResourceFinderTest, NoImport) {
  pagespeed::Resource* r = NewCssResource(kCssUrl);
  r->SetResponseBody(kNoImportBody);

  std::set<std::string> urls;
  ExternalResourceFinder f;
  f.FindExternalResources(*r, &urls);
  ASSERT_TRUE(urls.empty());
}

TEST_F(ExternalResourceFinderTest, BasicImport) {
  pagespeed::Resource* r = NewCssResource(kCssUrl);
  r->SetResponseBody(kBasicImportBody);

  std::set<std::string> urls;
  ExternalResourceFinder f;
  f.FindExternalResources(*r, &urls);
  ASSERT_EQ(1U, urls.size());
  ASSERT_EQ(kImportUrl1, *urls.begin());
}

TEST_F(ExternalResourceFinderTest, TwoBasicImports) {
  pagespeed::Resource* r = NewCssResource(kCssUrl);
  r->SetResponseBody(kTwoBasicImportsBody);

  std::set<std::string> urls;
  ExternalResourceFinder f;
  f.FindExternalResources(*r, &urls);
  ASSERT_EQ(2U, urls.size());
  ASSERT_EQ(kImportUrl1, *urls.begin());
  ASSERT_EQ(kImportUrl2, *urls.rbegin());
}

TEST_F(ExternalResourceFinderTest, TwoRelativeImports) {
  pagespeed::Resource* r = NewCssResource(kCssUrl);
  r->SetResponseBody(kTwoRelativeImportsBody);

  std::set<std::string> urls;
  ExternalResourceFinder f;
  f.FindExternalResources(*r, &urls);
  ASSERT_EQ(2U, urls.size());
  ASSERT_EQ(kImportUrl1, *urls.begin());
  ASSERT_EQ(kImportUrl2, *urls.rbegin());
}

TEST_F(ExternalResourceFinderTest, OneImport) {
  pagespeed::Resource* r = NewCssResource(kCssUrl);
  r->SetResponseBody(kOneImportBody);

  std::set<std::string> urls;
  ExternalResourceFinder f;
  f.FindExternalResources(*r, &urls);
  ASSERT_EQ(1U, urls.size());
  ASSERT_EQ(kImportUrl1, *urls.begin());
}

TEST_F(ExternalResourceFinderTest, NoImportInComment) {
  pagespeed::Resource* r = NewCssResource(kCssUrl);
  r->SetResponseBody(kImportInCommentBody);

  std::set<std::string> urls;
  ExternalResourceFinder f;
  f.FindExternalResources(*r, &urls);
  ASSERT_TRUE(urls.empty());
}

TEST_F(ExternalResourceFinderTest, NoImportUnterminatedComment) {
  pagespeed::Resource* r = NewCssResource(kCssUrl);
  r->SetResponseBody(kUnterminatedCommentBody);

  std::set<std::string> urls;
  ExternalResourceFinder f;
  f.FindExternalResources(*r, &urls);
  ASSERT_TRUE(urls.empty());
}

TEST_F(ExternalResourceFinderTest, BadUrlInImport) {
  pagespeed::Resource* r = NewCssResource(kCssUrl);
  r->SetResponseBody(kBadImportUrlBody);

  std::set<std::string> urls;
  ExternalResourceFinder f;
  f.FindExternalResources(*r, &urls);
  ASSERT_TRUE(urls.empty());
}

}  // namespace
