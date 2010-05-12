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
#include "pagespeed/rules/enable_gzip_compression.h"
#include "pagespeed/rules/savings_computer.h"
#include "testing/gtest/include/gtest/gtest.h"

using pagespeed::rules::compression_computer::ZlibComputer;
using pagespeed::rules::EnableGzipCompression;
using pagespeed::rules::SavingsComputer;
using pagespeed::PagespeedInput;
using pagespeed::Resource;
using pagespeed::Result;
using pagespeed::Results;
using pagespeed::ResultProvider;
using pagespeed::ResultVector;
using pagespeed::Savings;

namespace {

class EnableGzipCompressionTest : public ::testing::Test {
 protected:
  virtual void SetUp() {
    input_.reset(new PagespeedInput);
  }

  virtual void TearDown() {
    input_.reset();
  }

  void AddTestResource(const char* url,
                       const char* content_type,
                       const char* content_encoding,
                       const std::string& body) {
    Resource* resource = new Resource;
    resource->SetRequestUrl(url);
    resource->SetRequestMethod("GET");
    resource->SetRequestProtocol("HTTP");
    resource->SetResponseStatusCode(200);
    resource->SetResponseProtocol("HTTP/1.1");

    if (content_type != NULL) {
      resource->AddResponseHeader("Content-Type", content_type);
    }

    if (content_encoding != NULL) {
      resource->AddResponseHeader("Content-Encoding", content_encoding);
    }

    resource->SetResponseBody(body);
    input_->AddResource(resource);
  }

  void AddTestResource(const char* url,
                       const char* content_type,
                       const char* content_encoding,
                       size_t content_length) {
    std::string body;
    body.append(content_length, ' ');
    AddTestResource(url, content_type, content_encoding, body);
  }

  void AddFirstLargeHtmlResource(const char* charset,
                                 bool gzip) {
    std::string content_type = "text/html";
    if (charset != NULL) {
      content_type += "; charset=";
      content_type += charset;
    }
    AddTestResource("http://www.test.com/",
                    content_type.c_str(),
                    gzip ? "gzip" : NULL,
                    9000);
  }

  void AddFirstLargeHtmlResource(bool gzip) {
    return AddFirstLargeHtmlResource(NULL, gzip);
  }

  void AddSecondLargeHtmlResource(bool gzip) {
    AddTestResource("http://www.test.com/foo",
                    "text/html",
                    gzip ? "gzip" : NULL,
                    4500);
  }

  void AddShortHtmlResource() {
    AddTestResource("http://www.test.com/",
                    "text/html",
                    NULL,
                    10);
  }

  void CheckNoViolations() {
    CheckNoViolationsInternal(true);
  }

  void CheckErrorAndNoViolations() {
    CheckNoViolationsInternal(false);
  }

  void CheckOneViolation(int expected_savings, int score) {
    CheckOneViolationInternal(new ZlibComputer(),
                              expected_savings,
                              score,
                              true);
  }

  void CheckErrorAndOneViolation(SavingsComputer* computer,
                                 int expected_savings) {
    CheckOneViolationInternal(computer, expected_savings, -1, false);
  }

  void CheckTwoViolations(int first_expected_savings,
                          int second_expected_savings,
                          int score) {
    EnableGzipCompression gzip_rule(new ZlibComputer());

    Results results;
    ResultProvider provider(gzip_rule, &results);
    ASSERT_TRUE(gzip_rule.AppendResults(*input_, &provider));
    ASSERT_EQ(results.results_size(), 2);

    const Result& result0 = results.results(0);
    ASSERT_EQ(result0.savings().response_bytes_saved(), first_expected_savings);
    ASSERT_EQ(result0.resource_urls_size(), 1);
    ASSERT_EQ(result0.resource_urls(0), "http://www.test.com/");

    const Result& result1 = results.results(1);
    ASSERT_EQ(result1.savings().response_bytes_saved(),
              second_expected_savings);
    ASSERT_EQ(result1.resource_urls_size(), 1);
    ASSERT_EQ(result1.resource_urls(0), "http://www.test.com/foo");

    ResultVector result_vector;
    result_vector.push_back(&result0);
    result_vector.push_back(&result1);
    ASSERT_EQ(score, gzip_rule.ComputeScore(*input_->input_information(),
                                            result_vector));
  }

 private:
  void CheckNoViolationsInternal(bool expect_success) {
    EnableGzipCompression gzip_rule(new ZlibComputer());

    Results results;
    ResultProvider provider(gzip_rule, &results);
    ASSERT_EQ(expect_success, gzip_rule.AppendResults(*input_, &provider));
    ASSERT_EQ(results.results_size(), 0);
  }

  void CheckOneViolationInternal(SavingsComputer* computer,
                                 int expected_savings,
                                 int score,
                                 bool expect_success) {
    EnableGzipCompression gzip_rule(computer);

    Results results;
    ResultProvider provider(gzip_rule, &results);
    ASSERT_EQ(expect_success, gzip_rule.AppendResults(*input_, &provider));
    ASSERT_EQ(results.results_size(), 1);

    const Result& result = results.results(0);
    ASSERT_EQ(result.savings().response_bytes_saved(), expected_savings);
    ASSERT_EQ(result.resource_urls_size(), 1);
    ASSERT_EQ(result.resource_urls(0), "http://www.test.com/");

    if (expect_success) {
      ResultVector result_vector;
      result_vector.push_back(&result);
      ASSERT_EQ(score, gzip_rule.ComputeScore(*input_->input_information(),
                                              result_vector));
    }
  }

  scoped_ptr<PagespeedInput> input_;
};

TEST_F(EnableGzipCompressionTest, ViolationLargeHtmlNoGzip) {
  AddFirstLargeHtmlResource(false);

  CheckOneViolation(8956, 0);
}

TEST_F(EnableGzipCompressionTest, ViolationLargeHtmlUtf8NoGzip) {
  AddFirstLargeHtmlResource("utf-8", false);

  CheckOneViolation(8956, 0);
}

TEST_F(EnableGzipCompressionTest, NoViolationLargeHtmlGzip) {
  AddFirstLargeHtmlResource(true);

  CheckNoViolations();
}

TEST_F(EnableGzipCompressionTest, NoViolationSmallHtmlNoGzip) {
  AddShortHtmlResource();

  CheckNoViolations();
}

TEST_F(EnableGzipCompressionTest, NoViolationLargeNoContentTypeNoGzip) {
  AddTestResource("http://www.test.com/", NULL, NULL, 9000);

  CheckNoViolations();
}

TEST_F(EnableGzipCompressionTest, NoViolationLargeImageNoGzip) {
  AddTestResource("http://www.test.com/", "image/jpeg", NULL, 9000);

  CheckNoViolations();
}

TEST_F(EnableGzipCompressionTest, NoViolationLargeHtmlGzipSdch) {
  AddTestResource("http://www.test.com/", "text/html", "gzip,sdch", 9000);

  CheckNoViolations();
}

TEST_F(EnableGzipCompressionTest, NoViolationTwoHtmlGzip) {
  AddFirstLargeHtmlResource(true);
  AddSecondLargeHtmlResource(true);

  CheckNoViolations();
}

TEST_F(EnableGzipCompressionTest, OneViolationTwoHtmlNoGzip) {
  AddFirstLargeHtmlResource(false);
  AddSecondLargeHtmlResource(true);

  CheckOneViolation(8956, 33);
}

TEST_F(EnableGzipCompressionTest, TwoViolationsTwoHtmlNoGzip) {
  AddFirstLargeHtmlResource(false);
  AddSecondLargeHtmlResource(false);

  CheckTwoViolations(8956, 4460, 0);
}

TEST_F(EnableGzipCompressionTest, NullComputer) {
  ASSERT_DEATH(new EnableGzipCompression(NULL),
               "SavingsComputer must be non-null.");
}

TEST_F(EnableGzipCompressionTest, BinaryResponseBody) {
  std::string body;
  body.append(9000, ' ');
  body[0] = '\0';
  AddTestResource("http://www.test.com/", "text/html", NULL, body);
  CheckOneViolation(8955, 0);
}

class FailAtSpecifiedIndexComputer : public SavingsComputer {
 public:
  explicit FailAtSpecifiedIndexComputer(int index)
      : index_(index),
        counter_(0) {}
  ~FailAtSpecifiedIndexComputer() {}

  virtual bool ComputeSavings(const Resource& resource, Savings* savings) {
    savings->set_response_bytes_saved(10);
    return (counter_++ != index_);
  }

 private:
  const int index_;
  int counter_;
};

TEST_F(EnableGzipCompressionTest, FailedComputationsNotAddedToResults) {
  AddFirstLargeHtmlResource(false);
  AddSecondLargeHtmlResource(false);

  CheckErrorAndOneViolation(new FailAtSpecifiedIndexComputer(1), 10);
}

TEST_F(EnableGzipCompressionTest, FailedComputationsNotAddedToResults2) {
  AddSecondLargeHtmlResource(false);
  AddFirstLargeHtmlResource(false);

  CheckErrorAndOneViolation(new FailAtSpecifiedIndexComputer(0), 10);
}

}  // namespace
