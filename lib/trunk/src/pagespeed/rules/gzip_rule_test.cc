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
#include "pagespeed/core/pagespeed_input.pb.h"
#include "pagespeed/core/pagespeed_output.pb.h"
#include "pagespeed/core/proto_resource_utils.h"
#include "pagespeed/rules/gzip_details.pb.h"
#include "pagespeed/rules/gzip_rule.h"
#include "testing/gtest/include/gtest/gtest.h"

using pagespeed::GzipDetails;
using pagespeed::GzipRule;
using pagespeed::PagespeedInput;
using pagespeed::ProtoInput;
using pagespeed::ProtoResource;
using pagespeed::ProtoResourceUtils;
using pagespeed::Result;
using pagespeed::ResultDetails;
using pagespeed::Results;

namespace {

class GzipRuleTest : public ::testing::Test {
 protected:
  virtual void SetUp() {
    proto_input_.reset(new ProtoInput);
  }

  virtual void TearDown() {
    proto_input_.reset();
  }

  void AddTestResource(const char* url,
                       const char* content_type,
                       const char* content_encoding,
                       const char* content_length,
                       const char* body) {
    ProtoResource* proto_resource = proto_input_->add_resources();
    proto_resource->set_request_url(url);
    proto_resource->set_request_method("GET");
    proto_resource->set_request_protocol("HTTP");
    proto_resource->set_response_status_code(200);
    proto_resource->set_response_protocol("HTTP/1.1");

    if (content_type != NULL) {
      ProtoResourceUtils::AddResponseHeader(
          proto_resource, "Content-Type", content_type);
    }

    if (content_encoding != NULL) {
      ProtoResourceUtils::AddResponseHeader(
          proto_resource, "Content-Encoding", content_encoding);
    }

    if (content_length != NULL) {
      ProtoResourceUtils::AddResponseHeader(
          proto_resource, "Content-Length", content_length);
    }

    if (body != NULL) {
      proto_resource->set_response_body(body);
    }
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
                    "9000",
                    NULL);
  }

  void AddFirstLargeHtmlResource(bool gzip) {
    return AddFirstLargeHtmlResource(NULL, gzip);
  }

  void AddSecondLargeHtmlResource(bool gzip) {
    AddTestResource("http://www.test.com/foo",
                    "text/html",
                    gzip ? "gzip" : NULL,
                    "4500",
                    NULL);
  }

  void AddHtmlResourceWithBody(const char* body,
                               bool gzip) {
    AddTestResource("http://www.test.com/",
                    "text/html",
                    gzip ? "gzip" : NULL,
                    NULL,
                    body);
  }

  void AddShortHtmlResource() {
    AddTestResource("http://www.test.com/",
                    "text/html",
                    NULL,
                    "10",
                    NULL);
  }

  void CheckNoViolations() {
    PagespeedInput input(proto_input_.get());
    GzipRule gzip_rule;

    Results results;
    gzip_rule.AppendResults(input, &results);
    ASSERT_EQ(results.results_size(), 0);
  }

  void CheckOneViolation() {
    PagespeedInput input(proto_input_.get());
    GzipRule gzip_rule;

    Results results;
    gzip_rule.AppendResults(input, &results);
    ASSERT_EQ(results.results_size(), 1);

    const Result& result = results.results(0);
    ASSERT_EQ(result.savings().response_bytes_saved(), 6000);

    const ResultDetails& details = result.details();
    const GzipDetails& gzip_details = details.GetExtension(
        GzipDetails::message_set_extension);
    ASSERT_EQ(gzip_details.url_savings_size(), 1);
    ASSERT_EQ(gzip_details.url_savings(0).url(), "http://www.test.com/");
    ASSERT_EQ(gzip_details.url_savings(0).saved_bytes(), 6000);
  }

  void CheckTwoViolations() {
    PagespeedInput input(proto_input_.get());
    GzipRule gzip_rule;

    Results results;
    gzip_rule.AppendResults(input, &results);
    ASSERT_EQ(results.results_size(), 2);

    const Result& result0 = results.results(0);
    ASSERT_EQ(result0.savings().response_bytes_saved(), 6000);

    const ResultDetails& details0 = result0.details();
    const GzipDetails& gzip_details0 = details0.GetExtension(
        GzipDetails::message_set_extension);
    ASSERT_EQ(gzip_details0.url_savings_size(), 1);
    ASSERT_EQ(gzip_details0.url_savings(0).url(), "http://www.test.com/");
    ASSERT_EQ(gzip_details0.url_savings(0).saved_bytes(), 6000);

    const Result& result1 = results.results(1);
    ASSERT_EQ(result1.savings().response_bytes_saved(), 3000);

    const ResultDetails& details1 = result1.details();
    const GzipDetails& gzip_details1 = details1.GetExtension(
        GzipDetails::message_set_extension);
    ASSERT_EQ(gzip_details0.url_savings_size(), 1);
    ASSERT_EQ(gzip_details1.url_savings(0).url(), "http://www.test.com/foo");
    ASSERT_EQ(gzip_details1.url_savings(0).saved_bytes(), 3000);
  }

 private:
  scoped_ptr<ProtoInput> proto_input_;
};

TEST_F(GzipRuleTest, ViolationLargeHtmlNoGzip) {
  AddFirstLargeHtmlResource(false);

  CheckOneViolation();
}

TEST_F(GzipRuleTest, ViolationLargeHtmlUtf8NoGzip) {
  AddFirstLargeHtmlResource("utf-8", false);

  CheckOneViolation();
}

TEST_F(GzipRuleTest, NoViolationLargeHtmlGzip) {
  AddFirstLargeHtmlResource(true);

  CheckNoViolations();
}

TEST_F(GzipRuleTest, NoViolationSmallHtmlNoGzip) {
  AddShortHtmlResource();

  CheckNoViolations();
}

TEST_F(GzipRuleTest, NoViolationLargeNoContentTypeNoGzip) {
  AddTestResource("http://www.test.com/", NULL, NULL, "9000", NULL);

  CheckNoViolations();
}

TEST_F(GzipRuleTest, NoViolationLargeImageNoGzip) {
  AddTestResource("http://www.test.com/", "image/jpeg", NULL, "9000", NULL);

  CheckNoViolations();
}

TEST_F(GzipRuleTest, ViolationLargeHtmlNoGzipNoContentLength) {
  std::string body;
  body.resize(9000, 'a');
  AddHtmlResourceWithBody(body.c_str(), false);

  CheckOneViolation();
}

TEST_F(GzipRuleTest, NoViolationLargeHtmlGzipNoContentLength) {
  std::string body;
  body.resize(9000, 'a');
  AddHtmlResourceWithBody(body.c_str(), true);

  CheckNoViolations();
}

TEST_F(GzipRuleTest, NoViolationTwoHtmlGzip) {
  AddFirstLargeHtmlResource(true);
  AddSecondLargeHtmlResource(true);

  CheckNoViolations();
}

TEST_F(GzipRuleTest, OneViolationTwoHtmlNoGzip) {
  AddFirstLargeHtmlResource(false);
  AddSecondLargeHtmlResource(true);

  CheckOneViolation();
}

TEST_F(GzipRuleTest, TwoViolationsTwoHtmlNoGzip) {
  AddFirstLargeHtmlResource(false);
  AddSecondLargeHtmlResource(false);

  CheckTwoViolations();
}

}  // namespace
