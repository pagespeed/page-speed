// Copyright 2009 Google Inc. All Rights Reserved.
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

#include "pagespeed/css/cssmin.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

const char* kBeforeMinification =
    "/* This is a CSS file.  Hooray. */\n"
    "\n"
    "BODY {\n"
    "  border: 5px solid blue;\n"
    "  color: red /*two comments*/ /*in a row*/;\n"
    "}\n"
    "\n"
    "DIV.bg1  {\n"
    "  background-image : url( 'www.example.com/bg1.png' ) ;\n"
    " } \n"
    "DIV.bg2 {\n"
    "  background-image : url(\"www.example.com/bg2.png\");  \n"
    "}\n";

const char* kAfterMinification =
    "BODY{border:5px solid blue;color:red;}\n"
    "DIV.bg1{background-image:url('www.example.com/bg1.png');}\n"
    "DIV.bg2{background-image:url(\"www.example.com/bg2.png\");}";

// At one point, the URL
// http://aranet.vo.llnwd.net/o28/themes/css/araStyleReset.css
// returned the following response (invalid CSS) which caused us to
// trigger an assert on Windows. This test verifies that when we
// encounter data like this, we do not assert and we do not attempt to
// modify it.
const unsigned char kBadData[] = { 0xef, 0xbb, 0xbf, 0xba };

class CssminTest : public testing::Test {
 protected:
  void CheckMinification(const std::string& before, const std::string& after) {
    std::string output;
    ASSERT_TRUE(pagespeed::css::MinifyCss(before, &output));
    ASSERT_EQ(after, output);

    int minified_size = -1;
    ASSERT_TRUE(pagespeed::css::GetMinifiedCssSize(before, &minified_size));
    ASSERT_EQ(static_cast<int>(after.size()), minified_size);
  }
};

TEST_F(CssminTest, Basic) {
  CheckMinification(kBeforeMinification, kAfterMinification);
}

TEST_F(CssminTest, AlreadyMinified) {
  CheckMinification(kAfterMinification, kAfterMinification);
}

TEST_F(CssminTest, RunawayComment) {
  CheckMinification("BODY { color: red; } /* unclosed comment...*",
                    "BODY{color:red;}");
}

TEST_F(CssminTest, RunawayString) {
  CheckMinification("DIV { background-image: url('ain\\'t   no  /*end*/ quote",
                    "DIV{background-image:url('ain\\'t   no  /*end*/ quote");
}

TEST_F(CssminTest, InvalidCss) {
  std::string bad_data(reinterpret_cast<const char*>(kBadData),
                       sizeof(kBadData));
  ASSERT_TRUE(4 == bad_data.size());
  CheckMinification(bad_data, bad_data);
}

// See http://code.google.com/p/page-speed/issues/detail?id=313
TEST_F(CssminTest, SeparateStringsFromWords) {
  CheckMinification("body { font: 11px \"Bitstream Vera Sans Mono\" ; }",
                    "body{font:11px \"Bitstream Vera Sans Mono\";}");
}

// See http://code.google.com/p/page-speed/issues/detail?id=339
TEST_F(CssminTest, SeparateParensFromWords) {
  CheckMinification("div { background: url( 'bg.gif' ) no-repeat "
                    "left center; border-style: none; }",
                    "div{background:url('bg.gif') no-repeat "
                    "left center;border-style:none;}");
}

// See http://code.google.com/p/page-speed/issues/detail?id=265
TEST_F(CssminTest, SeparateBracketsFromWords1) {
  CheckMinification(".class[ rel ] { color: #f00; }\n"
                    ".class [rel] { color: #0f0; }",
                    ".class[rel]{color:#f00;}\n"
                    ".class [rel]{color:#0f0;}");
}

// See http://code.google.com/p/page-speed/issues/detail?id=379
TEST_F(CssminTest, SeparateBracketsFromWords2) {
  CheckMinification("body[class$=\"section\"] header {}",
                    "body[class$=\"section\"] header{}");
}

// See http://code.google.com/p/page-speed/issues/detail?id=381
TEST_F(CssminTest, SeparateBracketsFromPeriods) {
  CheckMinification("html[xmlns] .clearfix { display: block; }",
                    "html[xmlns] .clearfix{display:block;}");
}

// See http://code.google.com/p/page-speed/issues/detail?id=400
TEST_F(CssminTest, DoNotAddSpaceWhereThereWasNone) {
  CheckMinification("body{color:red;}h1{color:blue;}",
                    "body{color:red;}h1{color:blue;}");
}

// See http://code.google.com/p/page-speed/issues/detail?id=432
TEST_F(CssminTest, PreserveHackyComments) {
  CheckMinification("html>/**/body { color: blue; }",
                    "html>/**/body{color:blue;}");
}

// See http://code.google.com/p/page-speed/issues/detail?id=511
TEST_F(CssminTest, DoNotJoinTokensSeparatedByComment1) {
  CheckMinification(".foo /*comment*/.bar { color: blue; }",
                    ".foo .bar{color:blue;}");
  CheckMinification(".foo/*comment*/.bar { color: blue; }",
                    ".foo .bar{color:blue;}");
}

}  // namespace
