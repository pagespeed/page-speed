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
#include "pagespeed/core/result_provider.h"
#include "pagespeed/proto/pagespeed_output.pb.h"
#include "pagespeed/rules/serve_static_content_from_a_cookieless_domain.h"
#include "pagespeed/testing/pagespeed_test.h"

using pagespeed::rules::ServeStaticContentFromACookielessDomain;
using pagespeed::PagespeedInput;
using pagespeed::Resource;
using pagespeed::Result;
using pagespeed::Results;
using pagespeed::ResultProvider;

namespace {

class ServeStaticContentFromACookielessDomainTest : public ::pagespeed_testing::PagespeedTest {
 protected:
  void AddTestResource(const std::string& url,
                       const std::string& type,
                       const std::string& cookie) {
    Resource* resource = new Resource;
    resource->SetRequestUrl(url);
    resource->SetRequestMethod("GET");
    resource->SetRequestProtocol("HTTP/1.1");
    resource->SetResponseStatusCode(200);
    resource->SetResponseProtocol("HTTP/1.1");
    resource->SetResponseBody("Hello, world!");
    resource->AddResponseHeader("Content-Type", type);
    if (!cookie.empty()) {
      resource->AddRequestHeader("Cookie", cookie);
    }
    AddResource(resource);
  }

  void CheckNoViolations() {
    ServeStaticContentFromACookielessDomain rule;
    Results results;
    ResultProvider provider(rule, &results);
    ASSERT_TRUE(rule.AppendResults(*input(), &provider));
    ASSERT_EQ(results.results_size(), 0);
  }

  void CheckOneViolation(const std::string& url) {
    ServeStaticContentFromACookielessDomain rule;
    Results results;
    ResultProvider provider(rule, &results);
    ASSERT_TRUE(rule.AppendResults(*input(), &provider));
    ASSERT_EQ(results.results_size(), 1);
    const Result& result = results.results(0);
    ASSERT_EQ(result.resource_urls_size(), 1);
    ASSERT_EQ(result.resource_urls(0), url);
  }
};

TEST_F(ServeStaticContentFromACookielessDomainTest, NoProblems) {
  AddTestResource("http://www.example.com/index.html",
                  "text/html", "CHOCOLATE-CHIP");
  AddTestResource("http://static.example.com/styles.css",
                  "text/css", "");
  Freeze();
  CheckNoViolations();
}

TEST_F(ServeStaticContentFromACookielessDomainTest, OneViolation) {
  AddTestResource("http://www.example.com/index.html",
                  "text/html", "CHOCOLATE-CHIP");
  AddTestResource("http://static.example.com/styles.css",
                  "text/css", "OATMEAL-RAISIN");
  Freeze();
  CheckOneViolation("http://static.example.com/styles.css");
}

}  // namespace
