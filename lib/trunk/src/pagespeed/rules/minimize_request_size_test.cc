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

#include "base/scoped_ptr.h"
#include "base/stl_util-inl.h"  // for STLDeleteContainerPointers
#include "pagespeed/core/pagespeed_input.h"
#include "pagespeed/core/resource.h"
#include "pagespeed/core/result_provider.h"
#include "pagespeed/formatters/text_formatter.h"
#include "pagespeed/proto/pagespeed_output.pb.h"
#include "pagespeed/rules/minimize_request_size.h"
#include "pagespeed/testing/pagespeed_test.h"

using pagespeed::rules::MinimizeRequestSize;
using pagespeed::PagespeedInput;
using pagespeed::Resource;
using pagespeed::Result;
using pagespeed::Results;
using pagespeed::ResultProvider;

namespace {

std::string kDescription =
    "The requests for the following URLs don't fit in a single packet.  "
    "Reducing the size of these requests could reduce latency.\n";

class MinimizeRequestSizeTest : public ::pagespeed_testing::PagespeedTest {
 protected:
  void AddTestResource(
      const std::string& url,
      const std::map<std::string, std::string>& request_headers) {
    Resource* resource = new Resource;
    resource->SetRequestUrl(url);
    resource->SetRequestMethod("GET");
    resource->SetResponseStatusCode(200);

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
    Results results;
    {
      // compute results
      MinimizeRequestSize min_request_rule;
      ResultProvider provider(min_request_rule, &results);
      ASSERT_TRUE(min_request_rule.AppendResults(*input(), &provider));
    }

    {
      // Check formatted output
      pagespeed::ResultVector result_vector;
      for (int ii = 0; ii < results.results_size(); ++ii) {
        result_vector.push_back(&results.results(ii));
      }

      std::stringstream output;
      pagespeed::formatters::TextFormatter formatter(&output);
      MinimizeRequestSize dimensions_rule;
      dimensions_rule.FormatResults(result_vector, &formatter);
      EXPECT_STREQ(expected_output.c_str(), output.str().c_str());
    }

    {
      // Check contents of results object
      ASSERT_EQ(static_cast<size_t>(results.results_size()), expected.size());

      for (size_t idx = 0; idx < expected.size(); ++idx) {
        const Result& result = results.results(idx);
        ASSERT_EQ(result.resource_urls_size(), 1);
        EXPECT_EQ(expected[idx], result.resource_urls(0));
      }
    }
  }
};

TEST_F(MinimizeRequestSizeTest, NoViolationUnderThreshold) {
  std::map<std::string, std::string> request_headers;
  request_headers["Cookie"] = "foobar";
  request_headers["Referer"] = "http://www.test.com/";
  AddTestResource("http://www.test.com/logo.png",
                  request_headers);

  Freeze();
  CheckNoViolations();
}

TEST_F(MinimizeRequestSizeTest, LongCookieTest) {
  std::string expected_output =
      kDescription +
      "  http://www.test.com/logo.png has a request size of 1.5KiB\n"
      "    * Request URL: 28B\n"
      "    * Cookies: 1.4KiB\n"
      "    * Referer Url: 20B\n"
      "    * Other: 34B\n";

  std::map<std::string, std::string> request_headers;
  request_headers["Cookie"] = std::string(1450, 'a');
  request_headers["Referer"] = "http://www.test.com/";
  AddTestResource("http://www.test.com/logo.png",
                  request_headers);

  Freeze();
  CheckOneViolation("http://www.test.com/logo.png", expected_output);
}

TEST_F(MinimizeRequestSizeTest, LongRefererTest) {
  std::string expected_output =
      kDescription +
      "  http://www.test.com/logo.png has a request size of 1.5KiB\n"
      "    * Request URL: 28B\n"
      "    * Cookies: 0B\n"
      "    * Referer Url: 1.4KiB\n"
      "    * Other: 25B\n";

  std::map<std::string, std::string> request_headers;
  request_headers["Referer"] = "http://www.test.com/" + std::string(1450, 'a');
  AddTestResource("http://www.test.com/logo.png",
                  request_headers);

  Freeze();
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
      "    * Other: 25B\n";

  std::map<std::string, std::string> request_headers;
  request_headers["Referer"] = "http://www.test.com/";
  AddTestResource(url, request_headers);

  Freeze();
  CheckOneViolation(url, expected_output);
}

TEST_F(MinimizeRequestSizeTest, LongRefererTwoViolationTest) {
  std::string expected_output =
      kDescription +
      "  http://www.test.com/logo.png has a request size of 1.5KiB\n"
      "    * Request URL: 28B\n"
      "    * Cookies: 0B\n"
      "    * Referer Url: 1.4KiB\n"
      "    * Other: 25B\n"
      "  http://www.test.com/index.html has a request size of 1.5KiB\n"
      "    * Request URL: 30B\n"
      "    * Cookies: 0B\n"
      "    * Referer Url: 1.4KiB\n"
      "    * Other: 25B\n";

  std::map<std::string, std::string> request_headers;
  request_headers["Referer"] = "http://www.test.com/" + std::string(1450, 'a');

  std::string url1 = "http://www.test.com/logo.png";
  AddTestResource(url1, request_headers);

  std::string url2 = "http://www.test.com/index.html";
  AddTestResource(url2, request_headers);

  Freeze();
  CheckTwoViolations(url1, url2, expected_output);
}

}  // namespace
