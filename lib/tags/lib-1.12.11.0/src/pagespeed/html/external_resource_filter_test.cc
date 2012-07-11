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
#include <vector>

#include "net/instaweb/htmlparse/public/html_parse.h"
#include "net/instaweb/htmlparse/public/empty_html_filter.h"
#include "net/instaweb/util/public/google_message_handler.h"
#include "pagespeed/html/external_resource_filter.h"
#include "pagespeed/testing/pagespeed_test.h"

namespace {

static const char* kRootUrl = "http://www.example.com/";
static const char* kHtml =
    "<html><body>"
    "<script src='foo.js'></script>"
    "<script src='http://www.example.com/foo.js'></script>"
    "<link rel='stylesheet' href='http://www.example.com/foo.css'>"
    "</body></html>";

static const char* kHtmlNoRelNoSrc =
    "<html><body>"
    "<script></script>"
    "<script src='http://www.example.com/foo.js'></script>"
    "<link href='http://www.example.com/foo.css'>"
    "</body></html>";

static const char* kImgHtml =
    "<html><body>"
    "<img src='http://www.example.com/foo.png'>"
    "<img src='bar.png'>"
    "<img src='data:image/png;base64,iVBORw0KGgoAAAANSUhEUg...'>"
    "</body></html>";

static const char* kHtmlInputImage =
    "<html><body>"
    "<input type='image' src='http://www.example.com/foo.png'>"
    "</body></html>";

static const char* kHtmlObjectData =
    "<html><body>"
    "<object data='http://www.example.com/foo.png'>"
    "</body></html>";

static const char* kHtmlBodyBackground =
    "<html><body background='http://www.example.com/foo.png'>"
    "</body></html>";

static const char* kHtmlTableBackground =
    "<html><body><table background='http://www.example.com/foo.png'></table>"
    "</body></html>";

class ExternalResourceFilterTest : public pagespeed_testing::PagespeedTest {
 public:
  virtual void DoSetUp() {
    NewPrimaryResource(kRootUrl);
  }
};

TEST_F(ExternalResourceFilterTest, Basic) {
  net_instaweb::GoogleMessageHandler message_handler;
  net_instaweb::HtmlParse html_parse(&message_handler);
  pagespeed::html::ExternalResourceFilter filter(&html_parse);
  html_parse.AddFilter(&filter);

  html_parse.StartParse(kRootUrl);
  html_parse.ParseText(kHtml, strlen(kHtml));
  html_parse.FinishParse();

  std::vector<std::string> external_resource_urls;
  ASSERT_TRUE(filter.GetExternalResourceUrls(&external_resource_urls,
                                             document(),
                                             kRootUrl));

  ASSERT_EQ(2U, external_resource_urls.size());
  ASSERT_EQ("http://www.example.com/foo.css", external_resource_urls[0]);
  ASSERT_EQ("http://www.example.com/foo.js", external_resource_urls[1]);
}

TEST_F(ExternalResourceFilterTest, Img) {
  net_instaweb::GoogleMessageHandler message_handler;
  net_instaweb::HtmlParse html_parse(&message_handler);
  pagespeed::html::ExternalResourceFilter filter(&html_parse);
  html_parse.AddFilter(&filter);

  html_parse.StartParse(kRootUrl);
  html_parse.ParseText(kImgHtml, strlen(kImgHtml));
  html_parse.FinishParse();

  std::vector<std::string> external_resource_urls;
  ASSERT_TRUE(filter.GetExternalResourceUrls(&external_resource_urls,
                                             document(),
                                             kRootUrl));

  ASSERT_EQ(2U, external_resource_urls.size());
  ASSERT_EQ("http://www.example.com/bar.png", external_resource_urls[0]);
  ASSERT_EQ("http://www.example.com/foo.png", external_resource_urls[1]);
}

TEST_F(ExternalResourceFilterTest, NoRelShouldNotCrash) {
  net_instaweb::GoogleMessageHandler message_handler;
  net_instaweb::HtmlParse html_parse(&message_handler);
  pagespeed::html::ExternalResourceFilter filter(&html_parse);
  html_parse.AddFilter(&filter);

  html_parse.StartParse(kRootUrl);
  html_parse.ParseText(kHtmlNoRelNoSrc, strlen(kHtmlNoRelNoSrc));
  html_parse.FinishParse();

  std::vector<std::string> external_resource_urls;
  ASSERT_TRUE(filter.GetExternalResourceUrls(&external_resource_urls,
                                             document(),
                                             kRootUrl));
}

TEST_F(ExternalResourceFilterTest, InputImage) {
  net_instaweb::GoogleMessageHandler message_handler;
  net_instaweb::HtmlParse html_parse(&message_handler);
  pagespeed::html::ExternalResourceFilter filter(&html_parse);
  html_parse.AddFilter(&filter);

  html_parse.StartParse(kRootUrl);
  html_parse.ParseText(kHtmlInputImage, strlen(kHtmlInputImage));
  html_parse.FinishParse();

  std::vector<std::string> external_resource_urls;
  ASSERT_TRUE(filter.GetExternalResourceUrls(&external_resource_urls,
                                             document(),
                                             kRootUrl));
}

TEST_F(ExternalResourceFilterTest, ObjectData) {
  net_instaweb::GoogleMessageHandler message_handler;
  net_instaweb::HtmlParse html_parse(&message_handler);
  pagespeed::html::ExternalResourceFilter filter(&html_parse);
  html_parse.AddFilter(&filter);

  html_parse.StartParse(kRootUrl);
  html_parse.ParseText(kHtmlObjectData, strlen(kHtmlObjectData));
  html_parse.FinishParse();

  std::vector<std::string> external_resource_urls;
  ASSERT_TRUE(filter.GetExternalResourceUrls(&external_resource_urls,
                                             document(),
                                             kRootUrl));
}

TEST_F(ExternalResourceFilterTest, BodyBackground) {
  net_instaweb::GoogleMessageHandler message_handler;
  net_instaweb::HtmlParse html_parse(&message_handler);
  pagespeed::html::ExternalResourceFilter filter(&html_parse);
  html_parse.AddFilter(&filter);

  html_parse.StartParse(kRootUrl);
  html_parse.ParseText(kHtmlBodyBackground, strlen(kHtmlBodyBackground));
  html_parse.FinishParse();

  std::vector<std::string> external_resource_urls;
  ASSERT_TRUE(filter.GetExternalResourceUrls(&external_resource_urls,
                                             document(),
                                             kRootUrl));
}

TEST_F(ExternalResourceFilterTest, TableBackground) {
  net_instaweb::GoogleMessageHandler message_handler;
  net_instaweb::HtmlParse html_parse(&message_handler);
  pagespeed::html::ExternalResourceFilter filter(&html_parse);
  html_parse.AddFilter(&filter);

  html_parse.StartParse(kRootUrl);
  html_parse.ParseText(kHtmlTableBackground, strlen(kHtmlTableBackground));
  html_parse.FinishParse();

  std::vector<std::string> external_resource_urls;
  ASSERT_TRUE(filter.GetExternalResourceUrls(&external_resource_urls,
                                             document(),
                                             kRootUrl));
}

}  // namespace
