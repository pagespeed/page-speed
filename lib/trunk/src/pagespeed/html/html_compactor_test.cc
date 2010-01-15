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

#include "pagespeed/html/html_compactor.h"
#include "testing/gtest/include/gtest/gtest.h"

using pagespeed::html::HtmlCompactor;

namespace {

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
    "  Bar.\n"
    "  <!-- comment -->\n"
    "  <IMG src = 'baz.png' Alt='\"indeed\"'  />\n"
    "  <prE>\n"
    "    don't mess with  my whitespace   please\n"
    "  </pre>\n"
    "  <div empty=''></div>\n"
    "  <FORM mEtHoD=get>\n"
    "   <button type=submit disabled=disabled>\n"
    "   <!--[DO NOT REMOVE]-->\n"
    "   <button type=reset disabled=disabled>\n"
    "  </FORM>\n"
    " </boDy>\n"
    "</HTML>\n";

const char* kAfterMinification =
    "\n"
    "<title>foo</title>\n"
    "<style>BODY{color:green;}\n"
    "</style>\n"
    "<script>\n"
    "function increment(x){return x+1;}</script>\n"
    "<body>\n"
    "Bar.\n"
    "<img alt='\"indeed\"' src=baz.png />\n"
    "<pre>\n"
    "    don't mess with  my whitespace   please\n"
    "  </pre>\n"
    "<div empty=\"\"></div>\n"
    "<form>\n"
    "<button disabled>\n"
    "<!--[DO NOT REMOVE]-->\n"
    "<button disabled type=reset>\n"
    "</form>\n";

TEST(HtmlCompactorTest, Basic) {
  std::string output;
  ASSERT_TRUE(HtmlCompactor::CompactHtml(kBeforeMinification, &output));
  ASSERT_EQ(kAfterMinification, output);
}

TEST(HtmlCompactorTest, AlreadyMinified) {
  std::string output;
  ASSERT_TRUE(HtmlCompactor::CompactHtml(kAfterMinification, &output));
  ASSERT_EQ(kAfterMinification, output);
}

const char* kRunawayComment =
    "<HTML>\n"
    " <Head>  <title>foo</title></heAd>\n"
    " <body>\n"
    "  Bar.\n"
    "  <!-- this comment never gets closed!\n"
    "  <img src='baz.png' alt='\"indeed\"' />\n"
    " </body>\n"
    "</HTML>\n";

const char* kRunawayCommentMinified =
    "\n"
    "<title>foo</title>\n"
    "<body>\n"
    "Bar.\n";

TEST(HtmlCompactorTest, RunawayComment) {
  std::string output;
  ASSERT_TRUE(HtmlCompactor::CompactHtml(kRunawayComment, &output));
  ASSERT_EQ(kRunawayCommentMinified, output);
}

}  // namespace
