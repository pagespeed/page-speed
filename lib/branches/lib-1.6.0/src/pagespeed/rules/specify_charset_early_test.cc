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

#include "base/scoped_ptr.h"
#include "pagespeed/core/pagespeed_input.h"
#include "pagespeed/core/resource.h"
#include "pagespeed/proto/pagespeed_output.pb.h"
#include "pagespeed/rules/specify_charset_early.h"
#include "testing/gtest/include/gtest/gtest.h"

using pagespeed::rules::SpecifyCharsetEarly;
using pagespeed::PagespeedInput;
using pagespeed::Resource;
using pagespeed::Result;
using pagespeed::Results;

namespace {

const int kLateThresholdBytes = 2048;


class SpecifyCharsetEarlyTest : public ::testing::Test {
 protected:

  virtual void SetUp() {
    input_.reset(new PagespeedInput);
  }

  virtual void TearDown() {
    input_.reset();
  }

  void AddTestResource(const std::string &url,
                       const std::string &header_name,
                       const std::string &header_value,
                       const std::string &body) {
    Resource* resource = new Resource;
    resource->SetResponseStatusCode(200);
    resource->SetRequestUrl(url);
    if (!header_name.empty()) {
      resource->AddResponseHeader(header_name, header_value);
    }
    resource->SetResponseBody(body);
    input_->AddResource(resource);
  }

  void CheckNoViolations() {
    SpecifyCharsetEarly charset_rule;
    Results results;
    ASSERT_TRUE(charset_rule.AppendResults(*input_, &results));
    ASSERT_EQ(results.results_size(), 0);
  }

  void CheckOneViolation(const std::string &url) {
    SpecifyCharsetEarly charset_rule;
    Results results;
    ASSERT_TRUE(charset_rule.AppendResults(*input_, &results));
    ASSERT_EQ(results.results_size(), 1);
    const Result& result = results.results(0);
    ASSERT_EQ(result.savings().page_reflows_saved(), 1);
    ASSERT_EQ(result.resource_urls_size(), 1);
    ASSERT_EQ(result.resource_urls(0), url);
  }

 private:
  scoped_ptr<PagespeedInput> input_;
};

TEST_F(SpecifyCharsetEarlyTest, CharsetInHeader) {
  AddTestResource("http://www.example.com/hello.html",
                  "Content-Type",
                  "text/html; charset=utf-8",
                  "Hello world");
  CheckNoViolations();
}

TEST_F(SpecifyCharsetEarlyTest, CharsetEarlyInHtml) {
  std::string html = "<html><head><meta http-equiv=\"Content-Type\" "
                     "content=\"text/html;   charset= utf-8\"></head><body>"
                     "Hello world"
                     "</body></html>";
  AddTestResource("http://www.example.com/hello.html",
                  "",
                  "",
                  html);
  CheckNoViolations();
}

TEST_F(SpecifyCharsetEarlyTest, NoSpaceCharsetEarlyInHtml) {
  std::string html = "<html><head><meta http-equiv=\"Content-Type\" "
                     "content=\"text/html;charset= utf-8\"></head><body>"
                     "Hello world"
                     "</body></html>";
  AddTestResource("http://www.example.com/hello.html",
                  "",
                  "",
                  html);
  CheckNoViolations();
}

TEST_F(SpecifyCharsetEarlyTest, CharsetLateInHtml) {
  std::string html = "<html><body>"
                     "Hello world";

  // pad spaces to push the meta tag late
  html.append(kLateThresholdBytes, ' ');
  html.append("<meta http-equiv=\"Content-Type\" "
              "content=\"text/html; charset=utf-8\">");
  html.append("</body></html>");
  AddTestResource("http://www.example.com/hello.html",
                  "",
                  "",
                  html);
  CheckOneViolation("http://www.example.com/hello.html");
}


TEST_F(SpecifyCharsetEarlyTest, NotHtmlContent) {
  AddTestResource("http://www.example.com/hello.html",
                  "Content-Type",
                  "text/javascript",
                  "Hello, world!");
  CheckNoViolations();
}

TEST_F(SpecifyCharsetEarlyTest, MissingCharset) {
  AddTestResource("http://www.example.com/hello.html",
                  "Content-Type",
                  "text/html",
                  "Hello, world!");
  CheckOneViolation("http://www.example.com/hello.html");
}

TEST_F(SpecifyCharsetEarlyTest, MissingContentType) {
  AddTestResource("http://www.example.com/hello.html",
                  "",
                  "",
                  "Hello, world!");
  CheckOneViolation("http://www.example.com/hello.html");
}

TEST_F(SpecifyCharsetEarlyTest, EmptyContentType) {
  AddTestResource("http://www.example.com/hello.html",
                  "Content-Type",
                  "",
                  "Hello, world!");
  CheckOneViolation("http://www.example.com/hello.html");
}


}  // namespace
