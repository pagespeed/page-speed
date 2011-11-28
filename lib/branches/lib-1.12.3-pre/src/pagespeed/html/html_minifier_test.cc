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

#include "pagespeed/html/html_minifier.h"
#include "testing/gtest/include/gtest/gtest.h"

using pagespeed::html::HtmlMinifier;

namespace {

const char* kTestUrl = "http://www.example.com";

const char* kBeforeMinification =
    "<HTML>\n"
    " <Head>\n"
    "  <title>foo</title>\n"
    "  <style>\n"
    "    BODY {\n"
    "      color: green;\n"
    "    }\n"
    "  </style>\n"
    "  <script LANGUAGE=whatever>\n"
    "    function increment(x) {\n"
    "      return x + 1;\n"
    "    }\n"
    "  </script>\n"
    " </heAd>\n"
    " <Body>\n"
    "  Bar.<!-- comment -->\n"
    "  <IMG src = 'baz.png' Alt='\"indeed\"'  />\n"
    "  <prE>\n"
    "    don't mess with  my whitespace   please\n"
    "  </pre>\n"
    "  <a href=\"http://www.example.com/\">Trailing slash in URL</a>\n"
    "  <div empty=''></div>\n"
    "  <FORM mEtHoD=get>\n"
    "   <button type=submit disabled=disabled>\n"
    "   <!--[if IE]> DO NOT REMOVE <![endif]-->\n"
    "   <button type=reset disabled=disabled>\n"
    "  </FORM>\n"
    " </boDy>\n"
    "</HTML>\n";

const char* kAfterMinification =
    "<html>\n"
    "<head>\n"
    "<title>foo</title>\n"
    "<style>BODY{color:green;}</style>\n"
    "<script language=whatever>function increment(x){return x+1;}</script>\n"
    "</head>\n"
    "<body>\n"
    "Bar.\n"
    "<img src=baz.png alt='\"indeed\"'/>\n"
    "<pre>\n"
    "    don't mess with  my whitespace   please\n"
    "  </pre>\n"
    "<a href=\"http://www.example.com/\">Trailing slash in URL</a>\n"
    "<div empty=''></div>\n"
    "<form>\n"
    "<button type=submit disabled>\n"
    "<!--[if IE]> DO NOT REMOVE <![endif]-->\n"
    "<button type=reset disabled>\n"
    "</form>\n"
    "</body>\n"
    "</html>\n";

TEST(HtmlMinifierTest, Basic) {
  std::string output;
  HtmlMinifier minifier;
  ASSERT_TRUE(minifier.MinifyHtml(kTestUrl, kBeforeMinification, &output));
  ASSERT_EQ(kAfterMinification, output);
}

TEST(HtmlMinifierTest, AlreadyMinified) {
  std::string output;
  HtmlMinifier minifier;
  ASSERT_TRUE(minifier.MinifyHtml(kTestUrl, kAfterMinification, &output));
  ASSERT_EQ(kAfterMinification, output);
}

const char* kWithDoctype =
    "<!doctype html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\" "
    "\"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\">"
    "<HTML>\n"
    " <HEAD><TITLE>Foo</TITLE></HEAD>\n"
    " <BODY><INPUT type=\"checkbox\" checked=\"checked\" /></BODY>\n"
    "</HTML>\n";

const char* kWithDoctypeMinified =
    "<!doctype html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\" "
    "\"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\">"
    "<html>\n"
    "<head><title>Foo</title></head>\n"
    "<body><input type=\"checkbox\" checked=\"checked\"/></body>\n"
    "</html>\n";

TEST(HtmlMinifierTest, RespectDoctype) {
  std::string output;
  HtmlMinifier minifier;
  ASSERT_TRUE(minifier.MinifyHtml(kTestUrl, kWithDoctype, &output));
  ASSERT_EQ(kWithDoctypeMinified, output);
}

const char* kWithoutDoctype =
    "<HTML>\n"
    " <HEAD><TITLE>Foo</TITLE></HEAD>\n"
    " <BODY><INPUT type=\"checkbox\" checked=\"checked\" /></BODY>\n"
    "</HTML>\n";

const char* kWithoutDoctypeMinified =
    "<html>\n"
    "<head><title>Foo</title></head>\n"
    "<body><input type=checkbox checked /></body>\n"
    "</html>\n";

TEST(HtmlMinifierTest, NoDoctypeMoreAggressiveMinification) {
  std::string output;
  HtmlMinifier minifier;
  ASSERT_TRUE(minifier.MinifyHtml(kTestUrl, kWithoutDoctype, &output));
  ASSERT_EQ(kWithoutDoctypeMinified, output);
}

TEST(HtmlMinifierTest, SgmlCommentInScriptBlock) {
  const char* kInput =
      "<script><!--\n function foo() { bar(); } //--></script>";
  const char* kExpected = "<script>function foo(){bar();}</script>";
  std::string output;
  HtmlMinifier minifier;
  ASSERT_TRUE(minifier.MinifyHtml(kTestUrl, kInput, &output));
  ASSERT_EQ(kExpected, output);
}

TEST(HtmlMinifierTest, SgmlCommentInScriptBlockNoNewline) {
  const char* kInput = "<script><!-- function foo() { bar(); } //--></script>";
  const char* kExpected = "<script></script>";
  std::string output;
  HtmlMinifier minifier;
  ASSERT_TRUE(minifier.MinifyHtml(kTestUrl, kInput, &output));
  ASSERT_EQ(kExpected, output);
}

TEST(HtmlMinifierTest, SgmlCommentInScriptBlockWhitespace) {
  const char* kInput =
      "<script>  \t<!--\n function foo() { bar(); } //-->\t\n </script>";
  const char* kExpected = "<script>function foo(){bar();}</script>";
  std::string output;
  HtmlMinifier minifier;
  ASSERT_TRUE(minifier.MinifyHtml(kTestUrl, kInput, &output));
  ASSERT_EQ(kExpected, output);
}

TEST(HtmlMinifierTest, SgmlCommentInScriptBlockMiddle) {
  const char* kInput =
      "<script>var a;<!-- function foo() { bar(); } //--></script>";
  const char* kExpected =
      "<script>var a;</script>";
  std::string output;
  HtmlMinifier minifier;
  ASSERT_TRUE(minifier.MinifyHtml(kTestUrl, kInput, &output));
  ASSERT_EQ(kExpected, output);
}

TEST(HtmlMinifierTest, SgmlCommentInScriptBlockNoClose) {
  const char* kInput =
      "<script><!-- function foo() { bar(); } </script>";
  const char* kExpected =
      "<script></script>";
  std::string output;
  HtmlMinifier minifier;
  ASSERT_TRUE(minifier.MinifyHtml(kTestUrl, kInput, &output));
  ASSERT_EQ(kExpected, output);
}

TEST(HtmlMinifierTest, SgmlCommentInScriptWholeLine) {
  const char* kInput =
      "<script><!-- function foo(){bar();} //--> \n</script>";
  const char* kExpected =
      "<script></script>";
  std::string output;
  HtmlMinifier minifier;
  ASSERT_TRUE(minifier.MinifyHtml(kTestUrl, kInput, &output));
  ASSERT_EQ(kExpected, output);
}

TEST(HtmlMinifierTest, RemoveQuotesFromHtml5) {
  const char* kInput =
      "<!DOCTYPE html><div  class=\"foo\" >foobar</div>";
  const char* kExpected =
      "<!DOCTYPE html><div class=foo>foobar</div>";
  std::string output;
  HtmlMinifier minifier;
  ASSERT_TRUE(minifier.MinifyHtmlWithType(kTestUrl, "text/html",
                                          kInput, &output));
  ASSERT_EQ(kExpected, output);
}

TEST(HtmlMinifierTest, DoNotRemoveQuotesFromXhtml5) {
  const char* kInput =
      "<!DOCTYPE html><div  class=\"foo\" >foobar</div>";
  const char* kExpected =
      "<!DOCTYPE html><div class=\"foo\">foobar</div>";
  std::string output;
  HtmlMinifier minifier;
  ASSERT_TRUE(minifier.MinifyHtmlWithType(kTestUrl, "application/xhtml+xml",
                                          kInput, &output));
  ASSERT_EQ(kExpected, output);
}

}  // namespace
