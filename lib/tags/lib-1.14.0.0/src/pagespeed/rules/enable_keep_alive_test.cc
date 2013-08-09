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
#include <vector>

#include "pagespeed/rules/enable_keep_alive.h"
#include "pagespeed/testing/pagespeed_test.h"

namespace {

using pagespeed::rules::EnableKeepAlive;
using pagespeed_testing::FakeDomElement;

class Violation {
 public:
  Violation(int _expected_rt_savings,
            const std::string& _host,
            const std::vector<std::string>& _urls)
      : expected_rt_savings(_expected_rt_savings),
        host(_host),
        urls(_urls) {
  }

  int expected_rt_savings;
  std::string host;
  std::vector<std::string> urls;
};


class EnableKeepAliveTest
    : public ::pagespeed_testing::PagespeedRuleTest<EnableKeepAlive> {
 protected:
  static const char* kRootUrl;
  static const int kImgSizeBytes;

  virtual void DoSetUp() {
    NewPrimaryResource(kRootUrl);
    CreateHtmlHeadBodyElements();
  }

  pagespeed::Resource* CreatePngResource(
      const std::string& url, int size) {
    FakeDomElement* element;
    pagespeed::Resource* resource = NewPngResource(url, body(), &element);
    std::string response_body(size, 'x');
    resource->SetResponseBody(response_body);
    // Set default protocol to be HTTP/1.1.
    resource->SetResponseProtocol(pagespeed::HTTP_11);
    return resource;
  }

  void CheckFormattedOutput(const std::string& expected_output) {
    ASSERT_TRUE(AppendResults());
    EXPECT_EQ(expected_output, FormatResults());
  }

  void CheckExpectedViolations(const std::vector<Violation>& expected) {
    ASSERT_TRUE(AppendResults());
    ASSERT_EQ(expected.size(), static_cast<size_t>(num_results()));

    for (size_t idx = 0; idx < expected.size(); ++idx) {
      ASSERT_EQ(expected[idx].urls.size(),
                static_cast<size_t>(result(idx).resource_urls_size()));
      for (size_t jdx = 0; jdx < expected[idx].urls.size(); ++jdx) {
        EXPECT_EQ(expected[idx].urls[jdx], result(idx).resource_urls(jdx));
      }
    }

    if (!expected.empty()) {
      // If there are violations, make sure we have a non-zero impact.
      ASSERT_GT(ComputeRuleImpact(), 0.0);
    }
  }
};

const char* EnableKeepAliveTest::kRootUrl = "http://test.com/";
const int EnableKeepAliveTest::kImgSizeBytes = 50;

TEST_F(EnableKeepAliveTest, EmptyDom) {
  Freeze();
  std::vector<Violation> no_violations;
  CheckExpectedViolations(no_violations);
}

TEST_F(EnableKeepAliveTest, OneResourceNoEnableKeepAlive) {
  CreatePngResource("http://test1.com/image.png", kImgSizeBytes);
  Freeze();
  std::vector<Violation> no_violations;
  CheckExpectedViolations(no_violations);
}

TEST_F(EnableKeepAliveTest, OneResourceClose) {
  pagespeed::Resource* resource =
      CreatePngResource("http://test1.com/image.png", kImgSizeBytes);
  resource->AddResponseHeader("Connection", "close");
  Freeze();
  std::vector<Violation> no_violations;
  CheckExpectedViolations(no_violations);
}

TEST_F(EnableKeepAliveTest, TwoResource) {
  const std::string url1 = "http://test1.com/image1.png";
  const std::string url2 = "http://test1.com/image2.png";
  CreatePngResource(url1, kImgSizeBytes);
  CreatePngResource(url2, kImgSizeBytes);
  Freeze();
  std::vector<Violation> violations;
  CheckExpectedViolations(violations);
}

TEST_F(EnableKeepAliveTest, TwoResourceClose) {
  const std::string url1 = "http://test1.com/image1.png";
  const std::string url2 = "http://test1.com/image2.png";
  pagespeed::Resource* resource1 = CreatePngResource(url1, kImgSizeBytes);
  pagespeed::Resource* resource2 = CreatePngResource(url2, kImgSizeBytes);
  resource1->AddResponseHeader("Connection", "close");
  resource2->AddResponseHeader("Connection", "close");
  Freeze();
  std::vector<std::string> urls;
  urls.push_back(url1);
  urls.push_back(url2);
  std::vector<Violation> violations;
  violations.push_back(Violation(1, "test.com", urls));
  CheckExpectedViolations(violations);
}

TEST_F(EnableKeepAliveTest, TwoResourceHttp1_0) {
  const std::string url1 = "http://test1.com/image1.png";
  const std::string url2 = "http://test1.com/image2.png";
  pagespeed::Resource* resource1 = CreatePngResource(url1, kImgSizeBytes);
  pagespeed::Resource* resource2 = CreatePngResource(url2, kImgSizeBytes);
  resource1->SetResponseProtocol("HTTP/1.0");
  resource2->SetResponseProtocol("HTTP/1.0");
  Freeze();
  std::vector<std::string> urls;
  urls.push_back(url1);
  urls.push_back(url2);
  std::vector<Violation> violations;
  violations.push_back(Violation(1, "test.com", urls));
  CheckExpectedViolations(violations);
}

TEST_F(EnableKeepAliveTest, TwoResourceOneClose) {
  const std::string url1 = "http://test1.com/image1.png";
  const std::string url2 = "http://test1.com/image2.png";
  pagespeed::Resource* resource1 = CreatePngResource(url1, kImgSizeBytes);
  CreatePngResource(url2, kImgSizeBytes);
  resource1->AddResponseHeader("Connection", "close");
  Freeze();
  std::vector<std::string> urls;
  urls.push_back(url1);
  std::vector<Violation> violations;
  violations.push_back(Violation(1, "test.com", urls));
  CheckExpectedViolations(violations);
}



TEST_F(EnableKeepAliveTest, TwoResourcesEnableKeepAlive) {
  const std::string url1 = "http://test.com/image1.png";
  const std::string url2 = "http://test.com/image2.png";
  pagespeed::Resource* resource1 = CreatePngResource(url1, kImgSizeBytes);
  pagespeed::Resource* resource2 = CreatePngResource(url2, kImgSizeBytes);
  resource1->AddResponseHeader("Connection", "Keep-Alive");
  resource2->AddResponseHeader("Connection", "Keep-Alive");
  Freeze();
  std::vector<Violation> no_violations;
  CheckExpectedViolations(no_violations);
}

TEST_F(EnableKeepAliveTest, TwoResourcesEnableKeepAliveHttp1_0) {
  const std::string url1 = "http://test.com/image1.png";
  const std::string url2 = "http://test.com/image2.png";
  pagespeed::Resource* resource1 = CreatePngResource(url1, kImgSizeBytes);
  pagespeed::Resource* resource2 = CreatePngResource(url2, kImgSizeBytes);
  resource1->AddResponseHeader("Connection", "Keep-Alive");
  resource2->AddResponseHeader("Connection", "Keep-Alive");
  resource1->SetResponseProtocol("HTTP/1.0");
  resource2->SetResponseProtocol("HTTP/1.0");
  Freeze();
  std::vector<Violation> no_violations;
  CheckExpectedViolations(no_violations);
}


TEST_F(EnableKeepAliveTest, TwoDomains) {
  const std::string url1 = "http://test.com/image1.js";
  const std::string url2 = "http://test.com/image2.png";
  pagespeed::Resource* resource1 = CreatePngResource(url1, kImgSizeBytes);
  pagespeed::Resource* resource2 = CreatePngResource(url2, kImgSizeBytes);
  resource1->AddResponseHeader("Connection", "close");
  resource2->AddResponseHeader("Connection", "close");

  const std::string url2_1 = "http://test2.com/image1.js";
  const std::string url2_2 = "http://test2.com/image2.png";
  pagespeed::Resource* resource2_1 = CreatePngResource(url2_1, kImgSizeBytes);
  pagespeed::Resource* resource2_2 = CreatePngResource(url2_2, kImgSizeBytes);
  resource2_1->AddResponseHeader("Connection", "close");
  resource2_2->AddResponseHeader("Connection", "close");
  Freeze();
  std::vector<std::string> urls;
  urls.push_back(url1);
  urls.push_back(url2);
  std::vector<std::string> urls2;
  urls2.push_back(url2_1);
  urls2.push_back(url2_2);
  std::vector<Violation> violations;
  violations.push_back(Violation(1, "test.com", urls));
  violations.push_back(Violation(1, "test2.com", urls2));
  CheckExpectedViolations(violations);
}

TEST_F(EnableKeepAliveTest, FormatTest) {
  std::string expected =
      "The host test.com should enable Keep-Alive. It serves the following "
      "resources.\n"
      "  http://test.com/image1.png\n"
      "  http://test.com/image2.png\n";

  const std::string url1 = "http://test.com/image1.png";
  const std::string url2 = "http://test.com/image2.png";
  pagespeed::Resource* resource1 = CreatePngResource(url1, kImgSizeBytes);
  pagespeed::Resource* resource2 = CreatePngResource(url2, kImgSizeBytes);
  resource1->AddResponseHeader("Connection", "close");
  resource2->AddResponseHeader("Connection", "close");
  Freeze();
  CheckFormattedOutput(expected);
}

TEST_F(EnableKeepAliveTest, FormatNoOutputTest) {
  Freeze();
  CheckFormattedOutput("");
}

}  // namespace
