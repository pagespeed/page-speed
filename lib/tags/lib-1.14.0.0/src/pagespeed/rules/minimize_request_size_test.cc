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
#include <map>

#include "base/memory/scoped_ptr.h"
#include "base/stl_util.h"  // for STLDeleteContainerPointers
#include "pagespeed/core/pagespeed_input.h"
#include "pagespeed/core/resource.h"
#include "pagespeed/core/result_provider.h"
#include "pagespeed/proto/pagespeed_output.pb.h"
#include "pagespeed/rules/minimize_request_size.h"
#include "pagespeed/testing/pagespeed_test.h"

using pagespeed::rules::MinimizeRequestSize;
using pagespeed::PagespeedInput;
using pagespeed::Resource;
using pagespeed::Result;
using pagespeed::Results;
using pagespeed::ResultProvider;
using pagespeed_testing::PagespeedRuleTest;

namespace {

std::string kDescription =
    "The requests for the following URLs don't fit in a single packet.  "
    "Reducing the size of these requests could reduce latency.\n";

class MinimizeRequestSizeTest : public PagespeedRuleTest<MinimizeRequestSize> {
 protected:
  void AddTestResource(
      const std::string& url, const std::string& mimetype,
      const std::map<std::string, std::string>& request_headers) {
    Resource* resource = new Resource;
    resource->SetRequestUrl(url);
    resource->SetRequestMethod("GET");
    resource->SetResponseStatusCode(200);
    resource->AddResponseHeader("Content-Type", mimetype);

    for (std::map<std::string, std::string>::const_iterator
             it = request_headers.begin(),
             end = request_headers.end();
         it != end;
         ++it) {
      resource->AddRequestHeader(it->first, it->second);
    }
    AddResource(resource);
  }

  void CheckNoViolations() {
    CheckExpectedViolations(std::vector<std::string>(), "");
  }

  void CheckOneViolation(const std::string& violation_url,
                         const std::string& expected_output) {
    std::vector<std::string> expected;
    expected.push_back(violation_url);
    CheckExpectedViolations(expected, expected_output);
  }

  void CheckTwoViolations(const std::string& violation_url1,
                          const std::string& violation_url2,
                          const std::string& expected_output) {
    std::vector<std::string> expected;
    expected.push_back(violation_url1);
    expected.push_back(violation_url2);
    CheckExpectedViolations(expected, expected_output);
  }

 private:
  void CheckExpectedViolations(const std::vector<std::string>& expected,
                               const std::string& expected_output) {
    Freeze();
    ASSERT_TRUE(AppendResults());
    EXPECT_EQ(expected_output, FormatResults());

    ASSERT_EQ(static_cast<size_t>(num_results()), expected.size());

    for (size_t idx = 0; idx < expected.size(); ++idx) {
      const Result& result = this->result(idx);
      ASSERT_EQ(result.resource_urls_size(), 1);
      EXPECT_EQ(expected[idx], result.resource_urls(0));
    }
  }
};

TEST_F(MinimizeRequestSizeTest, NoViolationUnderThreshold) {
  std::map<std::string, std::string> request_headers;
  request_headers["Cookie"] = "foobar";
  request_headers["Referer"] = "http://www.test.com/";
  AddTestResource("http://www.test.com/logo.png", "image/png",
                  request_headers);
  CheckNoViolations();
}

TEST_F(MinimizeRequestSizeTest, LongCookieTest) {
  std::string expected_output =
      kDescription +
      "  http://www.test.com/logo.png has a request size of 1.5KiB\n"
      "    * Request URL: 28B\n"
      "    * Cookies: 1.4KiB (note that this is a static resource, and should "
      "be served from a cookieless domain)\n"
      "    * Referer Url: 20B\n"
      "    * Other: 36B\n"
      "  http://www.test.com/foo.html has a request size of 1.5KiB\n"
      "    * Request URL: 28B\n"
      "    * Cookies: 1.4KiB\n"
      "    * Referer Url: 20B\n"
      "    * Other: 36B\n";

  std::map<std::string, std::string> request_headers;
  request_headers["Cookie"] = std::string(1450, 'a');
  request_headers["Referer"] = "http://www.test.com/";

  const std::string url1 = "http://www.test.com/logo.png";
  AddTestResource(url1, "image/png", request_headers);

  const std::string url2 = "http://www.test.com/foo.html";
  AddTestResource(url2, "text/html", request_headers);

  CheckTwoViolations(url1, url2, expected_output);
}

TEST_F(MinimizeRequestSizeTest, LongRefererTest) {
  std::string expected_output =
      kDescription +
      "  http://www.test.com/logo.png has a request size of 1.5KiB\n"
      "    * Request URL: 28B\n"
      "    * Cookies: 0B\n"
      "    * Referer Url: 1.4KiB\n"
      "    * Other: 27B\n";

  std::map<std::string, std::string> request_headers;
  request_headers["Referer"] = "http://www.test.com/" + std::string(1450, 'a');
  AddTestResource("http://www.test.com/logo.png", "image/png",
                  request_headers);

  CheckOneViolation("http://www.test.com/logo.png", expected_output);
}

TEST_F(MinimizeRequestSizeTest, LongUrlTest) {
  std::string url = "http://www.test.com/" + std::string(1450, 'a');
  std::string expected_output =
      kDescription +
      "  " + url + " has a request size of 1.5KiB\n"
      "    * Request URL: 1.4KiB\n"
      "    * Cookies: 0B\n"
      "    * Referer Url: 20B\n"
      "    * Other: 27B\n";

  std::map<std::string, std::string> request_headers;
  request_headers["Referer"] = "http://www.test.com/";
  AddTestResource(url, "text/html", request_headers);

  CheckOneViolation(url, expected_output);
}

TEST_F(MinimizeRequestSizeTest, LongRefererTwoViolationTest) {
  std::string expected_output =
      kDescription +
      "  http://www.test.com/logo.png has a request size of 1.5KiB\n"
      "    * Request URL: 28B\n"
      "    * Cookies: 0B\n"
      "    * Referer Url: 1.4KiB\n"
      "    * Other: 27B\n"
      "  http://www.test.com/index.html has a request size of 1.5KiB\n"
      "    * Request URL: 30B\n"
      "    * Cookies: 0B\n"
      "    * Referer Url: 1.4KiB\n"
      "    * Other: 27B\n";

  std::map<std::string, std::string> request_headers;
  request_headers["Referer"] = "http://www.test.com/" + std::string(1450, 'a');

  const std::string url1 = "http://www.test.com/logo.png";
  AddTestResource(url1, "image/png", request_headers);

  const std::string url2 = "http://www.test.com/index.html";
  AddTestResource(url2, "text/html", request_headers);

  CheckTwoViolations(url1, url2, expected_output);
}

}  // namespace
