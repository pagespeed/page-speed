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
static const char* kImgUrl1 = "http://www.example.com/background.png";

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

static const char* kBackgroundImgBody =
    "body {background-image:url('http://www.example.com/background.png');}";

class AvoidCssImportTest
    : public ::pagespeed_testing::PagespeedRuleTest<AvoidCssImport> {
 protected:
  const pagespeed::AvoidCssImportDetails& details(int result_idx) {
    pagespeed_testing::AssertTrue(result(result_idx).has_details());
    const pagespeed::ResultDetails& details = result(result_idx).details();
    pagespeed_testing::AssertTrue(details.HasExtension(
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
  NewCssResource(kImportUrl1);
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
  NewCssResource(kImportUrl1);
  NewCssResource(kImportUrl2);
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
  NewCssResource(kImportUrl1);
  NewCssResource(kImportUrl2);
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
  NewCssResource(kImportUrl1);
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

// Make sure non-CSS resources referenced from the CSS body are not
// included in the result set.
TEST_F(AvoidCssImportTest, BackgroundImage) {
  NewCssResource(kCssUrl)->SetResponseBody(kBackgroundImgBody);
  NewPngResource(kImgUrl1);
  Freeze();
  AppendResults();
  ASSERT_EQ(0, num_results());
}

}  // namespace
