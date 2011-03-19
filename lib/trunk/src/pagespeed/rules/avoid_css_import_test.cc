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

#include "pagespeed/proto/pagespeed_output.pb.h"
#include "pagespeed/rules/avoid_css_import.h"
#include "pagespeed/testing/pagespeed_test.h"

namespace {

using pagespeed::AvoidCssImportDetails;
using pagespeed::rules::AvoidCssImport;

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
  AvoidCssImport::RemoveComments("", &css);
  ASSERT_TRUE(css.empty());
}

TEST(RemoveCssCommentsTest, NoComments) {
  const char* kNoComments = "here is some text that does not contain comments";
  std::string css;
  AvoidCssImport::RemoveComments(kNoComments, &css);
  ASSERT_EQ(css, kNoComments);
}

TEST(RemoveCssCommentsTest, EmptyComment) {
  const char* kComment = "/**/";
  std::string css;
  AvoidCssImport::RemoveComments(kComment, &css);
  ASSERT_TRUE(css.empty());
}

TEST(RemoveCssCommentsTest, EmptyComments) {
  const char* kComment = "/**//**//**//**/";
  std::string css;
  AvoidCssImport::RemoveComments(kComment, &css);
  ASSERT_TRUE(css.empty());
}

TEST(RemoveCssCommentsTest, SimpleComment) {
  const char* kComment = "/* here is a comment*/";
  std::string css;
  AvoidCssImport::RemoveComments(kComment, &css);
  ASSERT_TRUE(css.empty());
}

TEST(RemoveCssCommentsTest, CommentAtBeginning) {
  const char* kComment = "/* here is a comment*/ content";
  std::string css;
  AvoidCssImport::RemoveComments(kComment, &css);
  ASSERT_EQ(" content", css);
}

TEST(RemoveCssCommentsTest, CommentAtEnd) {
  const char* kComment = "content /* here is a comment*/";
  std::string css;
  AvoidCssImport::RemoveComments(kComment, &css);
  ASSERT_EQ("content ", css);
}

TEST(RemoveCssCommentsTest, CommentAtBothEnds) {
  const char* kComment = "/* comment*/ content /* here is a comment*/";
  std::string css;
  AvoidCssImport::RemoveComments(kComment, &css);
  ASSERT_EQ(" content ", css);
}

TEST(RemoveCssCommentsTest, CommentInMiddle) {
  const char* kComment = "content /* comment*/ content";
  std::string css;
  AvoidCssImport::RemoveComments(kComment, &css);
  ASSERT_EQ("content  content", css);
}

TEST(RemoveCssCommentsTest, MultiLineComment) {
  const char* kComment = "/*here\nis\na\ncomment*/";
  std::string css;
  AvoidCssImport::RemoveComments(kComment, &css);
  ASSERT_TRUE(css.empty());
}

TEST(RemoveCssCommentsTest, MultipleComments) {
  const char* kComment = "/* here is a comment*//*here is another*/";
  std::string css;
  AvoidCssImport::RemoveComments(kComment, &css);
  ASSERT_TRUE(css.empty());
}

TEST(RemoveCssCommentsTest, MultipleCommentsContentBetween) {
  const char* kComment =
      "here /* here is a comment*/ is /*here is another*/ content";
  std::string css;
  AvoidCssImport::RemoveComments(kComment, &css);
  ASSERT_EQ(css, "here  is  content");
}

TEST(RemoveCssCommentsTest, MultipleMultiLineCommentsContentBetween) {
  const char* kComment =
      "here\n /*\nhere\nis\na\ncomment*/ is /*here\nis\nanother*/ \ncontent";
  std::string css;
  AvoidCssImport::RemoveComments(kComment, &css);
  ASSERT_EQ(css, "here\n  is  \ncontent");
}

TEST(RemoveCssCommentsTest, UnterminatedComment) {
  const char* kComment = "/*an unterminated comment";
  std::string css;
  AvoidCssImport::RemoveComments(kComment, &css);
  ASSERT_TRUE(css.empty());
}

TEST(RemoveCssCommentsTest, UnterminatedComment2) {
  const char* kComment = "here  is  content/*an unterminated comment";
  std::string css;
  AvoidCssImport::RemoveComments(kComment, &css);
  ASSERT_EQ(css, "here  is  content");
}

TEST(RemoveCssCommentsTest, UnterminatedComment3) {
  const char* kComment =
      "here/* */  is/* comment*/  content/*an unterminated comment";
  std::string css;
  AvoidCssImport::RemoveComments(kComment, &css);
  ASSERT_EQ(css, "here  is  content");
}

// Comments aren't allowed to be nested. Verify that we handle nested
// comments correctly. See
// http://www.w3.org/TR/CSS21/syndata.html#comments for more.
TEST(RemoveCssCommentsTest, NestedComment) {
  const char* kComment =
      "here  is  content /* here is /* a nested */ comment */";
  std::string css;
  AvoidCssImport::RemoveComments(kComment, &css);
  ASSERT_EQ(css, "here  is  content  comment */");
}

TEST(IsCssImportLineTest, String) {
  std::string url;
  ASSERT_FALSE(AvoidCssImport::IsCssImportLine("", &url));
  ASSERT_FALSE(AvoidCssImport::IsCssImportLine("foo {};", &url));
  ASSERT_FALSE(AvoidCssImport::IsCssImportLine("@iMpOrT", &url));
  ASSERT_FALSE(AvoidCssImport::IsCssImportLine("@iMpOrT", &url));
  ASSERT_FALSE(AvoidCssImport::IsCssImportLine("@iMpOrT \"", &url));
  ASSERT_FALSE(AvoidCssImport::IsCssImportLine("@iMpOrT '", &url));
  ASSERT_FALSE(AvoidCssImport::IsCssImportLine("@iMpOrT \"\"", &url));
  ASSERT_FALSE(AvoidCssImport::IsCssImportLine("@iMpOrT ''", &url));
  ASSERT_FALSE(AvoidCssImport::IsCssImportLine("@iMpOrT '\"", &url));
  ASSERT_FALSE(AvoidCssImport::IsCssImportLine("@iMpOrT \"'", &url));

  // Should not match if end quote is missing.
  ASSERT_FALSE(AvoidCssImport::IsCssImportLine(
      "@iMpOrT 'http://www.example.com/foo.css", &url));

  // Mismatched quotes for URL should not match.
  ASSERT_FALSE(AvoidCssImport::IsCssImportLine(
      "@iMpOrT 'http://www.example.com/foo.css\"", &url));

  // Test with single quotes.
  ASSERT_TRUE(AvoidCssImport::IsCssImportLine(
      "@iMpOrT 'http://www.example.com/foo.css'", &url));
  ASSERT_EQ("http://www.example.com/foo.css", url);

  // Test with double quotes.
  ASSERT_TRUE(AvoidCssImport::IsCssImportLine(
      "@iMpOrT \"http://www.example.com/foo.css\"", &url));
  ASSERT_EQ("http://www.example.com/foo.css", url);

  // Relative URL
  ASSERT_TRUE(AvoidCssImport::IsCssImportLine("@iMpOrT 'foo.css'", &url));
  ASSERT_EQ("foo.css", url);

  // Relative URL
  ASSERT_TRUE(AvoidCssImport::IsCssImportLine("@iMpOrT 'foo.css'", &url));
  ASSERT_EQ("foo.css", url);

  // No space
  ASSERT_TRUE(AvoidCssImport::IsCssImportLine("@iMpOrT'foo.css'", &url));
  ASSERT_EQ("foo.css", url);

  // Many spaces
  ASSERT_TRUE(AvoidCssImport::IsCssImportLine("@iMpOrT   'foo.css'", &url));
  ASSERT_EQ("foo.css", url);

  // Whitespace at ends of URL spaces (we do not trim).
  ASSERT_TRUE(AvoidCssImport::IsCssImportLine("@iMpOrT   ' foo.css '", &url));
  ASSERT_EQ(" foo.css ", url);
}

TEST(IsCssImportLineTest, Url) {
  std::string url;

  // No URL.
  ASSERT_FALSE(AvoidCssImport::IsCssImportLine("@iMpOrT UrL(", &url));
  ASSERT_FALSE(AvoidCssImport::IsCssImportLine("@iMpOrT UrL('", &url));
  ASSERT_FALSE(AvoidCssImport::IsCssImportLine("@iMpOrT UrL(''", &url));
  ASSERT_FALSE(AvoidCssImport::IsCssImportLine("@iMpOrT UrL('\"", &url));
  ASSERT_FALSE(AvoidCssImport::IsCssImportLine("@iMpOrT UrL()", &url));
  ASSERT_FALSE(AvoidCssImport::IsCssImportLine("@iMpOrT UrL(')", &url));
  ASSERT_FALSE(AvoidCssImport::IsCssImportLine("@iMpOrT UrL(\")", &url));
  ASSERT_FALSE(AvoidCssImport::IsCssImportLine("@iMpOrT UrL('')", &url));
  ASSERT_FALSE(AvoidCssImport::IsCssImportLine("@iMpOrT UrL(\"\")", &url));
  ASSERT_FALSE(AvoidCssImport::IsCssImportLine("@iMpOrT UrL('\")", &url));
  ASSERT_FALSE(AvoidCssImport::IsCssImportLine("@iMpOrT UrL(\"')", &url));

  // No space, non-terminated parenthesis.
  ASSERT_FALSE(AvoidCssImport::IsCssImportLine("@iMpOrTUrL('foo.css'", &url));

  // One space, non-terminated parenthesis.
  ASSERT_FALSE(AvoidCssImport::IsCssImportLine("@iMpOrT UrL('foo.css'", &url));

  // Multi spaces, non-terminated parenthesis.
  ASSERT_FALSE(AvoidCssImport::IsCssImportLine("@iMpOrT  UrL('foo.css'", &url));

  // One space, non-terminated parenthesis, no quotes.
  ASSERT_FALSE(AvoidCssImport::IsCssImportLine("@iMpOrT UrL(foo.css", &url));

  // Mismatched quotes for URL should not match.
  ASSERT_FALSE(AvoidCssImport::IsCssImportLine(
      "@iMpOrT uRl('http://www.example.com/foo.css\")", &url));

  // No space.
  ASSERT_TRUE(AvoidCssImport::IsCssImportLine("@iMpOrTUrL('foo.css')", &url));
  ASSERT_EQ("foo.css", url);

  // One space.
  ASSERT_TRUE(AvoidCssImport::IsCssImportLine("@iMpOrT UrL('foo.css')", &url));
  ASSERT_EQ("foo.css", url);

  // Multi spaces.
  ASSERT_TRUE(AvoidCssImport::IsCssImportLine("@iMpOrT  UrL('foo.css')", &url));
  ASSERT_EQ("foo.css", url);

  // Extra spaces.
  ASSERT_TRUE(
      AvoidCssImport::IsCssImportLine("@iMpOrT  UrL(' foo.css ')", &url));
  ASSERT_EQ(" foo.css ", url);

  // No quotes.
  ASSERT_TRUE(AvoidCssImport::IsCssImportLine("@iMpOrT UrL(foo.css)", &url));
  ASSERT_EQ("foo.css", url);

  // Extra spaces.
  ASSERT_TRUE(AvoidCssImport::IsCssImportLine("@iMpOrT UrL( foo.css )", &url));
  ASSERT_EQ("foo.css", url);

  // Short
  ASSERT_TRUE(AvoidCssImport::IsCssImportLine("@iMpOrT UrL(a)", &url));
  ASSERT_EQ("a", url);
}

class AvoidCssImportTest
    : public ::pagespeed_testing::PagespeedRuleTest<AvoidCssImport> {
 protected:
  void AssertTrue(bool statement) { ASSERT_TRUE(statement); }

  const pagespeed::AvoidCssImportDetails& details(int result_idx) {
    AssertTrue(result(result_idx).has_details());
    const pagespeed::ResultDetails& details = result(result_idx).details();
    AssertTrue(details.HasExtension(
        AvoidCssImportDetails::message_set_extension));
    return details.GetExtension(AvoidCssImportDetails::message_set_extension);
  }
};

TEST_F(AvoidCssImportTest, EmptyBody) {
  NewCssResource(kCssUrl);
  Freeze();
  AppendResults();
  ASSERT_EQ(0, num_results());
}

TEST_F(AvoidCssImportTest, NoImport) {
  NewCssResource(kCssUrl)->SetResponseBody(kNoImportBody);
  Freeze();
  AppendResults();
  ASSERT_EQ(0, num_results());
}

TEST_F(AvoidCssImportTest, BasicImport) {
  NewCssResource(kCssUrl)->SetResponseBody(kBasicImportBody);
  Freeze();
  AppendResults();
  ASSERT_EQ(1, num_results());
  ASSERT_EQ(1, result(0).resource_urls_size());
  ASSERT_EQ(kCssUrl, result(0).resource_urls(0));
  ASSERT_EQ(1, details(0).imported_stylesheets_size());
  ASSERT_EQ(kImportUrl1, details(0).imported_stylesheets(0));
}

TEST_F(AvoidCssImportTest, TwoBasicImports) {
  NewCssResource(kCssUrl)->SetResponseBody(kTwoBasicImportsBody);
  Freeze();
  AppendResults();
  ASSERT_EQ(1, num_results());
  ASSERT_EQ(1, result(0).resource_urls_size());
  ASSERT_EQ(kCssUrl, result(0).resource_urls(0));
  ASSERT_EQ(2, details(0).imported_stylesheets_size());
  ASSERT_EQ(kImportUrl1, details(0).imported_stylesheets(0));
  ASSERT_EQ(kImportUrl2, details(0).imported_stylesheets(1));
}

TEST_F(AvoidCssImportTest, TwoRelativeImports) {
  NewCssResource(kCssUrl)->SetResponseBody(kTwoRelativeImportsBody);
  Freeze();
  AppendResults();
  ASSERT_EQ(1, num_results());
  ASSERT_EQ(1, result(0).resource_urls_size());
  ASSERT_EQ(kCssUrl, result(0).resource_urls(0));
  ASSERT_EQ(2, details(0).imported_stylesheets_size());
  ASSERT_EQ(kImportUrl1, details(0).imported_stylesheets(0));
  ASSERT_EQ(kImportUrl2, details(0).imported_stylesheets(1));
}

TEST_F(AvoidCssImportTest, OneImport) {
  NewCssResource(kCssUrl)->SetResponseBody(kOneImportBody);
  Freeze();
  AppendResults();
  ASSERT_EQ(1, num_results());
  ASSERT_EQ(1, result(0).resource_urls_size());
  ASSERT_EQ(kCssUrl, result(0).resource_urls(0));
  ASSERT_EQ(1, details(0).imported_stylesheets_size());
  ASSERT_EQ(kImportUrl1, details(0).imported_stylesheets(0));
}

TEST_F(AvoidCssImportTest, NoImportInComment) {
  NewCssResource(kCssUrl)->SetResponseBody(kImportInCommentBody);
  Freeze();
  AppendResults();
  ASSERT_EQ(0, num_results());
}

TEST_F(AvoidCssImportTest, NoImportUnterminatedComment) {
  NewCssResource(kCssUrl)->SetResponseBody(kUnterminatedCommentBody);
  Freeze();
  AppendResults();
  ASSERT_EQ(0, num_results());
}

TEST_F(AvoidCssImportTest, BadUrlInImport) {
  NewCssResource(kCssUrl)->SetResponseBody(kBadImportUrlBody);
  Freeze();
  AppendResults();
  ASSERT_EQ(0, num_results());
}

}  // namespace
