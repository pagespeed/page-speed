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

using pagespeed::css::CssTokenizer;
using pagespeed::css::FindExternalResourcesInCssResource;
using pagespeed::css::RemoveCssComments;

struct CssToken {
  CssTokenizer::CssTokenType type;
  const char* token;
};

static const char* kCssUrl = "http://www.example.com/foo.css";
static const char* kImportUrl1 = "http://www.example.com/import1.css";
static const char* kImportUrl2 = "http://www.example.com/import2.css";

static const char* kNoImportBody =
    "body {\n"
    "color: purple;\n"
    "background-color: #d8da3d }";

static const CssToken kNoImportBodyTokens[] = {
  { CssTokenizer::IDENT, "body" },
  { CssTokenizer::SEPARATOR, "{" },
  { CssTokenizer::IDENT, "color" },
  { CssTokenizer::SEPARATOR, ":" },
  { CssTokenizer::IDENT, "purple" },
  { CssTokenizer::SEPARATOR, ";" },
  { CssTokenizer::IDENT, "background-color" },
  { CssTokenizer::SEPARATOR, ":" },
  { CssTokenizer::IDENT, "#d8da3d" },
  { CssTokenizer::SEPARATOR, "}" },
};

static const size_t kNoImportBodyTokensLen =
    sizeof(kNoImportBodyTokens) / sizeof(kNoImportBodyTokens[0]);

static const char* kBasicImportBody =
    "@import \" http://www.example.com/import1.css \"";

static const CssToken kBasicImportBodyTokens[] = {
  { CssTokenizer::IDENT, "@import" },
  { CssTokenizer::STRING, " http://www.example.com/import1.css " },
};

static const size_t kBasicImportBodyTokensLen =
    sizeof(kBasicImportBodyTokens) / sizeof(kBasicImportBodyTokens[0]);

static const char* kTwoBasicImportsBody =
    "@import url(\"http://www.example.com/import1.css\")\n"
    "@import url(\"http://www.example.com/import2.css\")";

static const CssToken kTwoBasicImportsBodyTokens[] = {
  { CssTokenizer::IDENT, "@import" },
  { CssTokenizer::URL, "http://www.example.com/import1.css" },
  { CssTokenizer::IDENT, "@import" },
  { CssTokenizer::URL, "http://www.example.com/import2.css" },
};

static const size_t kTwoBasicImportsBodyTokensLen =
    sizeof(kTwoBasicImportsBodyTokens) / sizeof(kTwoBasicImportsBodyTokens[0]);

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
  RemoveCssComments("", &css);
  ASSERT_TRUE(css.empty());
}

TEST(RemoveCssCommentsTest, NoComments) {
  const char* kNoComments = "here is some text that does not contain comments";
  std::string css;
  RemoveCssComments(kNoComments, &css);
  ASSERT_EQ(css, kNoComments);
}

TEST(RemoveCssCommentsTest, EmptyComment) {
  const char* kComment = "/**/";
  std::string css;
  RemoveCssComments(kComment, &css);
  ASSERT_TRUE(css.empty());
}

TEST(RemoveCssCommentsTest, EmptyComments) {
  const char* kComment = "/**//**//**//**/";
  std::string css;
  RemoveCssComments(kComment, &css);
  ASSERT_TRUE(css.empty());
}

TEST(RemoveCssCommentsTest, SimpleComment) {
  const char* kComment = "/* here is a comment*/";
  std::string css;
  RemoveCssComments(kComment, &css);
  ASSERT_TRUE(css.empty());
}

TEST(RemoveCssCommentsTest, CommentAtBeginning) {
  const char* kComment = "/* here is a comment*/ content";
  std::string css;
  RemoveCssComments(kComment, &css);
  ASSERT_EQ(" content", css);
}

TEST(RemoveCssCommentsTest, CommentAtEnd) {
  const char* kComment = "content /* here is a comment*/";
  std::string css;
  RemoveCssComments(kComment, &css);
  ASSERT_EQ("content ", css);
}

TEST(RemoveCssCommentsTest, CommentAtBothEnds) {
  const char* kComment = "/* comment*/ content /* here is a comment*/";
  std::string css;
  RemoveCssComments(kComment, &css);
  ASSERT_EQ(" content ", css);
}

TEST(RemoveCssCommentsTest, CommentInMiddle) {
  const char* kComment = "content /* comment*/ content";
  std::string css;
  RemoveCssComments(kComment, &css);
  ASSERT_EQ("content  content", css);
}

TEST(RemoveCssCommentsTest, MultiLineComment) {
  const char* kComment = "/*here\nis\na\ncomment*/";
  std::string css;
  RemoveCssComments(kComment, &css);
  ASSERT_TRUE(css.empty());
}

TEST(RemoveCssCommentsTest, MultipleComments) {
  const char* kComment = "/* here is a comment*//*here is another*/";
  std::string css;
  RemoveCssComments(kComment, &css);
  ASSERT_TRUE(css.empty());
}

TEST(RemoveCssCommentsTest, MultipleCommentsContentBetween) {
  const char* kComment =
      "here /* here is a comment*/ is /*here is another*/ content";
  std::string css;
  RemoveCssComments(kComment, &css);
  ASSERT_EQ(css, "here  is  content");
}

TEST(RemoveCssCommentsTest, MultipleMultiLineCommentsContentBetween) {
  const char* kComment =
      "here\n /*\nhere\nis\na\ncomment*/ is /*here\nis\nanother*/ \ncontent";
  std::string css;
  RemoveCssComments(kComment, &css);
  ASSERT_EQ(css, "here\n  is  \ncontent");
}

TEST(RemoveCssCommentsTest, UnterminatedComment) {
  const char* kComment = "/*an unterminated comment";
  std::string css;
  RemoveCssComments(kComment, &css);
  ASSERT_TRUE(css.empty());
}

TEST(RemoveCssCommentsTest, UnterminatedComment2) {
  const char* kComment = "here  is  content/*an unterminated comment";
  std::string css;
  RemoveCssComments(kComment, &css);
  ASSERT_EQ(css, "here  is  content");
}

TEST(RemoveCssCommentsTest, UnterminatedComment3) {
  const char* kComment =
      "here/* */  is/* comment*/  content/*an unterminated comment";
  std::string css;
  RemoveCssComments(kComment, &css);
  ASSERT_EQ(css, "here  is  content");
}

// Comments aren't allowed to be nested. Verify that we handle nested
// comments correctly. See
// http://www.w3.org/TR/CSS21/syndata.html#comments for more.
TEST(RemoveCssCommentsTest, NestedComment) {
  const char* kComment =
      "here  is  content /* here is /* a nested */ comment */";
  std::string css;
  RemoveCssComments(kComment, &css);
  ASSERT_EQ(css, "here  is  content  comment */");
}

TEST(CssTokenizerTest, Empty) {
  CssTokenizer tokenizer("");
  std::string token;
  CssTokenizer::CssTokenType type;
  ASSERT_FALSE(tokenizer.GetNextToken(&token, &type));
}

TEST(CssTokenizerTest, Whitespace) {
  CssTokenizer tokenizer("   \n  \t  ");
  std::string token;
  CssTokenizer::CssTokenType type;
  ASSERT_FALSE(tokenizer.GetNextToken(&token, &type));
}

TEST(CssTokenizerTest, OneIdentToken) {
  CssTokenizer tokenizer("   a   ");
  std::string token;
  CssTokenizer::CssTokenType type;
  ASSERT_TRUE(tokenizer.GetNextToken(&token, &type));
  ASSERT_STREQ("a", token.c_str());
  ASSERT_EQ(CssTokenizer::IDENT, type);
  ASSERT_FALSE(tokenizer.GetNextToken(&token, &type));
}

TEST(CssTokenizerTest, OneUrlToken) {
  CssTokenizer tokenizer("url('foo.bar')");
  std::string token;
  CssTokenizer::CssTokenType type;
  ASSERT_TRUE(tokenizer.GetNextToken(&token, &type));
  ASSERT_STREQ("foo.bar", token.c_str());
  ASSERT_EQ(CssTokenizer::URL, type);
  ASSERT_FALSE(tokenizer.GetNextToken(&token, &type));
}

TEST(CssTokenizerTest, OneUrlTokenUnterminated) {
  CssTokenizer tokenizer("url('foo.bar'");
  std::string token;
  CssTokenizer::CssTokenType type;
  ASSERT_TRUE(tokenizer.GetNextToken(&token, &type));
  ASSERT_STREQ("url('foo.bar'", token.c_str());
  ASSERT_EQ(CssTokenizer::INVALID, type);
  ASSERT_FALSE(tokenizer.GetNextToken(&token, &type));
}

TEST(CssTokenizerTest, OneUrlTokenMissingClosingBrace) {
  CssTokenizer tokenizer("url('foo.bar'}");
  std::string token;
  CssTokenizer::CssTokenType type;
  ASSERT_TRUE(tokenizer.GetNextToken(&token, &type));
  ASSERT_STREQ("url('foo.bar'}", token.c_str());
  ASSERT_EQ(CssTokenizer::INVALID, type);
  ASSERT_FALSE(tokenizer.GetNextToken(&token, &type));
}

TEST(CssTokenizerTest, UrlTokenMisplacedCloseParen) {
  CssTokenizer tokenizer("url('foo.bar'} div { foo: bar })  'string'");
  std::string token;
  CssTokenizer::CssTokenType type;
  ASSERT_TRUE(tokenizer.GetNextToken(&token, &type));
  ASSERT_STREQ("url('foo.bar'} div { foo: bar })", token.c_str());
  ASSERT_EQ(CssTokenizer::INVALID, type);
  ASSERT_TRUE(tokenizer.GetNextToken(&token, &type));
  ASSERT_STREQ("string", token.c_str());
  ASSERT_EQ(CssTokenizer::STRING, type);
  ASSERT_FALSE(tokenizer.GetNextToken(&token, &type));
}

TEST(CssTokenizerTest, OneUrlTokenCloseParenInUrl) {
  CssTokenizer tokenizer("url('foo).bar')");
  std::string token;
  CssTokenizer::CssTokenType type;
  ASSERT_TRUE(tokenizer.GetNextToken(&token, &type));
  ASSERT_STREQ("foo).bar", token.c_str());
  ASSERT_EQ(CssTokenizer::URL, type);
  ASSERT_FALSE(tokenizer.GetNextToken(&token, &type));
}

TEST(CssTokenizerTest, OneUrlTokenEscapedQuote) {
  CssTokenizer tokenizer("url('foo\\'.bar')");
  std::string token;
  CssTokenizer::CssTokenType type;
  ASSERT_TRUE(tokenizer.GetNextToken(&token, &type));
  ASSERT_STREQ("foo'.bar", token.c_str());
  ASSERT_EQ(CssTokenizer::URL, type);
  ASSERT_FALSE(tokenizer.GetNextToken(&token, &type));
}

TEST(CssTokenizerTest, OneUrlTokenNoQuotes) {
  CssTokenizer tokenizer("url(foo.bar)");
  std::string token;
  CssTokenizer::CssTokenType type;
  ASSERT_TRUE(tokenizer.GetNextToken(&token, &type));
  ASSERT_STREQ("foo.bar", token.c_str());
  ASSERT_EQ(CssTokenizer::URL, type);
  ASSERT_FALSE(tokenizer.GetNextToken(&token, &type));
}

TEST(CssTokenizerTest, OneUrlTokenNoQuotesSpaces) {
  CssTokenizer tokenizer("url(  foo.bar\n  )");
  std::string token;
  CssTokenizer::CssTokenType type;
  ASSERT_TRUE(tokenizer.GetNextToken(&token, &type));
  ASSERT_STREQ("foo.bar", token.c_str());
  ASSERT_EQ(CssTokenizer::URL, type);
  ASSERT_FALSE(tokenizer.GetNextToken(&token, &type));
}

TEST(CssTokenizerTest, OneUrlTokenSpaces) {
  CssTokenizer tokenizer("  url(   \n  'foo.bar'   \r\t  \n )  ");
  std::string token;
  CssTokenizer::CssTokenType type;
  ASSERT_TRUE(tokenizer.GetNextToken(&token, &type));
  ASSERT_STREQ("foo.bar", token.c_str());
  ASSERT_EQ(CssTokenizer::URL, type);
  ASSERT_FALSE(tokenizer.GetNextToken(&token, &type));
}

TEST(CssTokenizerTest, UnterminatedUrlToken) {
  CssTokenizer tokenizer("  url(   \n  'foo.bar");
  std::string token;
  CssTokenizer::CssTokenType type;
  ASSERT_TRUE(tokenizer.GetNextToken(&token, &type));
  ASSERT_STREQ("url(   \n  'foo.bar", token.c_str());
  ASSERT_EQ(CssTokenizer::INVALID, type);
  ASSERT_FALSE(tokenizer.GetNextToken(&token, &type));
}

TEST(CssTokenizerTest, OneStringToken) {
  CssTokenizer tokenizer("   ' here is a string'  ");
  std::string token;
  CssTokenizer::CssTokenType type;
  ASSERT_TRUE(tokenizer.GetNextToken(&token, &type));
  ASSERT_STREQ(" here is a string", token.c_str());
  ASSERT_EQ(CssTokenizer::STRING, type);
  ASSERT_FALSE(tokenizer.GetNextToken(&token, &type));
}

void ExpectOneStringToken(const char* input, const char* expected) {
  CssTokenizer tokenizer(input);
  std::string token;
  CssTokenizer::CssTokenType type;
  ASSERT_TRUE(tokenizer.GetNextToken(&token, &type));
  ASSERT_STREQ(expected, token.c_str());
  ASSERT_EQ(CssTokenizer::STRING, type);
  ASSERT_FALSE(tokenizer.GetNextToken(&token, &type));
}

TEST(CssTokenizerTest, OneEscapedStringToken) {
  ExpectOneStringToken("   ' here \\\r\nis a \\' \\\r string \\\n \\\\'  ",
                       " here is a '  string  \\");
  ExpectOneStringToken("   ' here is a \\\\ string'  ",
                       " here is a \\ string");
  ExpectOneStringToken("   ' here is a \\\r string'  ",
                       " here is a  string");
  ExpectOneStringToken("   ' here is a \\\r\n string'  ",
                       " here is a  string");
  ExpectOneStringToken("   ' here is a \\\n string'  ",
                       " here is a  string");
  ExpectOneStringToken("   ' here is a \\\" string'  ",
                       " here is a \" string");
  ExpectOneStringToken("   \" here is a \\\" string\"  ",
                       " here is a \" string");
  ExpectOneStringToken("   \" here is a \\\' string\"  ",
                       " here is a \' string");
}

TEST(CssTokenizerTest, UnterminatedString) {
  CssTokenizer tokenizer("   ' here is a string  ");
  std::string token;
  CssTokenizer::CssTokenType type;
  ASSERT_TRUE(tokenizer.GetNextToken(&token, &type));
  ASSERT_STREQ(" here is a string  ", token.c_str());
  ASSERT_EQ(CssTokenizer::STRING, type);
  ASSERT_FALSE(tokenizer.GetNextToken(&token, &type));
}

TEST(CssTokenizerTest, UnterminatedString2) {
  CssTokenizer tokenizer("   ' here is a string  \nfoo 'bar'");
  std::string token;
  CssTokenizer::CssTokenType type;
  ASSERT_TRUE(tokenizer.GetNextToken(&token, &type));
  ASSERT_STREQ(" here is a string  ", token.c_str());
  ASSERT_EQ(CssTokenizer::STRING, type);
  ASSERT_TRUE(tokenizer.GetNextToken(&token, &type));
  ASSERT_STREQ("foo", token.c_str());
  ASSERT_EQ(CssTokenizer::IDENT, type);
  ASSERT_TRUE(tokenizer.GetNextToken(&token, &type));
  ASSERT_STREQ("bar", token.c_str());
  ASSERT_EQ(CssTokenizer::STRING, type);
  ASSERT_FALSE(tokenizer.GetNextToken(&token, &type));
}

TEST(CssTokenizerTest, NoImportTokens) {
  CssTokenizer tokenizer(kNoImportBody);
  std::string token;
  CssTokenizer::CssTokenType type;
  const CssToken* expected = kNoImportBodyTokens;
  while (tokenizer.GetNextToken(&token, &type)) {
    ASSERT_STREQ(expected->token, token.c_str());
    ASSERT_EQ(expected->type, type);
    ++expected;
  }
  ASSERT_EQ(kNoImportBodyTokensLen,
            static_cast<size_t>(expected - kNoImportBodyTokens));
}

TEST(CssTokenizerTest, BasicImportTokens) {
  CssTokenizer tokenizer(kBasicImportBody);
  std::string token;
  CssTokenizer::CssTokenType type;
  const CssToken* expected = kBasicImportBodyTokens;
  while (tokenizer.GetNextToken(&token, &type)) {
    ASSERT_STREQ(expected->token, token.c_str());
    ASSERT_EQ(expected->type, type);
    ++expected;
  }
  ASSERT_EQ(kBasicImportBodyTokensLen,
            static_cast<size_t>(expected - kBasicImportBodyTokens));
}

TEST(CssTokenizerTest, TwoBasicImportsTokens) {
  CssTokenizer tokenizer(kTwoBasicImportsBody);
  std::string token;
  CssTokenizer::CssTokenType type;
  const CssToken* expected = kTwoBasicImportsBodyTokens;
  while (tokenizer.GetNextToken(&token, &type)) {
    ASSERT_STREQ(expected->token, token.c_str());
    ASSERT_EQ(expected->type, type);
    ++expected;
  }
  ASSERT_EQ(kTwoBasicImportsBodyTokensLen,
            static_cast<size_t>(expected - kTwoBasicImportsBodyTokens));
}

// Helper method that inserts all substrings of a given body starting
// at the first character, to make sure the tokenizer doesn't have
// trouble parsing incomplete tokens. Here we are not testing for
// token correctness but rather making sure that partial inputs don't
// cause crashes.
void StressCssTokenizer(const std::string& body) {
  std::string token;
  CssTokenizer::CssTokenType type;
  for (size_t i = 0; i < body.length(); ++i) {
    CssTokenizer tokenizer(body.substr(0, i));
    while (tokenizer.GetNextToken(&token, &type)) {}
  }
}

TEST(CssTokenizerTest, Stress) {
  StressCssTokenizer(kNoImportBody);
  StressCssTokenizer(kBasicImportBody);
  StressCssTokenizer(kTwoBasicImportsBody);
  StressCssTokenizer(kTwoRelativeImportsBody);
}

class ExternalResourceFinderTest : public pagespeed_testing::PagespeedTest {};

TEST_F(ExternalResourceFinderTest, EmptyBody) {
  pagespeed::Resource* r = NewCssResource(kCssUrl);

  std::set<std::string> urls;
  FindExternalResourcesInCssResource(*r, &urls);
  ASSERT_TRUE(urls.empty());
}

TEST_F(ExternalResourceFinderTest, NoImport) {
  pagespeed::Resource* r = NewCssResource(kCssUrl);
  r->SetResponseBody(kNoImportBody);

  std::set<std::string> urls;
  FindExternalResourcesInCssResource(*r, &urls);
  ASSERT_TRUE(urls.empty());
}

TEST_F(ExternalResourceFinderTest, BasicImport) {
  pagespeed::Resource* r = NewCssResource(kCssUrl);
  r->SetResponseBody(kBasicImportBody);

  std::set<std::string> urls;
  FindExternalResourcesInCssResource(*r, &urls);
  ASSERT_EQ(1U, urls.size());
  ASSERT_EQ(kImportUrl1, *urls.begin());
}

TEST_F(ExternalResourceFinderTest, TwoBasicImports) {
  pagespeed::Resource* r = NewCssResource(kCssUrl);
  r->SetResponseBody(kTwoBasicImportsBody);

  std::set<std::string> urls;
  FindExternalResourcesInCssResource(*r, &urls);
  ASSERT_EQ(2U, urls.size());
  ASSERT_EQ(kImportUrl1, *urls.begin());
  ASSERT_EQ(kImportUrl2, *urls.rbegin());
}

TEST_F(ExternalResourceFinderTest, TwoRelativeImports) {
  pagespeed::Resource* r = NewCssResource(kCssUrl);
  r->SetResponseBody(kTwoRelativeImportsBody);

  std::set<std::string> urls;
  FindExternalResourcesInCssResource(*r, &urls);
  ASSERT_EQ(2U, urls.size());
  ASSERT_EQ(kImportUrl1, *urls.begin());
  ASSERT_EQ(kImportUrl2, *urls.rbegin());
}

TEST_F(ExternalResourceFinderTest, OneImport) {
  pagespeed::Resource* r = NewCssResource(kCssUrl);
  r->SetResponseBody(kOneImportBody);

  std::set<std::string> urls;
  FindExternalResourcesInCssResource(*r, &urls);
  ASSERT_EQ(1U, urls.size());
  ASSERT_EQ(kImportUrl1, *urls.begin());
}

TEST_F(ExternalResourceFinderTest, NoImportInComment) {
  pagespeed::Resource* r = NewCssResource(kCssUrl);
  r->SetResponseBody(kImportInCommentBody);

  std::set<std::string> urls;
  FindExternalResourcesInCssResource(*r, &urls);
  ASSERT_TRUE(urls.empty());
}

TEST_F(ExternalResourceFinderTest, NoImportUnterminatedComment) {
  pagespeed::Resource* r = NewCssResource(kCssUrl);
  r->SetResponseBody(kUnterminatedCommentBody);

  std::set<std::string> urls;
  FindExternalResourcesInCssResource(*r, &urls);
  ASSERT_TRUE(urls.empty());
}

TEST_F(ExternalResourceFinderTest, BadUrlInImport) {
  pagespeed::Resource* r = NewCssResource(kCssUrl);
  r->SetResponseBody(kBadImportUrlBody);

  std::set<std::string> urls;
  FindExternalResourcesInCssResource(*r, &urls);
  ASSERT_TRUE(urls.empty());
}

}  // namespace
