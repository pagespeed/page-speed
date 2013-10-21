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

#include "base/memory/scoped_ptr.h"
#include "pagespeed/core/pagespeed_input.h"
#include "pagespeed/core/resource.h"
#include "pagespeed/core/result_provider.h"
#include "pagespeed/proto/pagespeed_output.pb.h"
#include "pagespeed/rules/enable_gzip_compression.h"
#include "pagespeed/rules/savings_computer.h"
#include "pagespeed/testing/pagespeed_test.h"

using pagespeed::rules::EnableGzipCompression;
using pagespeed::rules::SavingsComputer;
using pagespeed::PagespeedInput;
using pagespeed::Resource;
using pagespeed::Result;
using pagespeed::Results;
using pagespeed::ResultProvider;
using pagespeed::ResultVector;
using pagespeed::RuleInput;
using pagespeed::RuleResults;
using pagespeed::Savings;

namespace {

class EnableGzipCompressionTest :
      public ::pagespeed_testing::PagespeedRuleTest<EnableGzipCompression> {
 protected:
  void AddTestResource(const char* url,
                       const char* content_type,
                       const char* content_encoding,
                       const std::string& body) {
    Resource* resource = New200Resource(url);
    if (content_type != NULL) {
      resource->AddResponseHeader("Content-Type", content_type);
    }

    if (content_encoding != NULL) {
      resource->AddResponseHeader("Content-Encoding", content_encoding);
    }

    resource->SetResponseBody(body);
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
};

TEST_F(EnableGzipCompressionTest, ViolationLargeHtmlNoGzip) {
  AddFirstLargeHtmlResource(false);
  CheckOneUrlViolation("http://www.test.com/");
  ASSERT_EQ(8956, result(0).savings().response_bytes_saved());
}

TEST_F(EnableGzipCompressionTest, ViolationLargeHtmlUtf8NoGzip) {
  AddFirstLargeHtmlResource("utf-8", false);
  CheckOneUrlViolation("http://www.test.com/");
  ASSERT_EQ(8956, result(0).savings().response_bytes_saved());
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

// See http://code.google.com/p/page-speed/issues/detail?id=487
TEST_F(EnableGzipCompressionTest, ViolationSvgXmlImageNoGzip) {
  AddTestResource("http://www.test.com/", "image/svg+xml", NULL, 9000);
  // TODO(mdsteele): We need this next line here to keep the score computer
  // from dying when it finds that the max_possible_cost is zero, because there
  // are no compressible bytes, because ComputeCompressibleResponseBytes
  // doesn't count SVG images.
  AddSecondLargeHtmlResource(true);

  CheckOneUrlViolation("http://www.test.com/");
  ASSERT_EQ(8956, result(0).savings().response_bytes_saved());
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
  CheckOneUrlViolation("http://www.test.com/");
  ASSERT_EQ(8956, result(0).savings().response_bytes_saved());
}

TEST_F(EnableGzipCompressionTest, TwoViolationsTwoHtmlNoGzip) {
  AddFirstLargeHtmlResource(false);
  AddSecondLargeHtmlResource(false);
  CheckTwoUrlViolations("http://www.test.com/", "http://www.test.com/foo");
  ASSERT_EQ(8956, result(0).savings().response_bytes_saved());
  ASSERT_EQ(4460, result(1).savings().response_bytes_saved());
}

TEST_F(EnableGzipCompressionTest, BinaryResponseBody) {
  std::string body;
  body.append(9000, ' ');
  body[0] = '\0';
  AddTestResource("http://www.test.com/", "text/html", NULL, body);
  CheckOneUrlViolation("http://www.test.com/");
  ASSERT_EQ(8955, result(0).savings().response_bytes_saved());
}

TEST_F(EnableGzipCompressionTest, Format) {
  AddFirstLargeHtmlResource(false);
  CheckOneUrlViolation("http://www.test.com/");
  ASSERT_EQ("Compressing resources with gzip or deflate can reduce "
            "the number of bytes sent over the network.\n"
            "Enable compression<https://developers.google.com/speed/docs/"
            "insights/EnableCompression> for the following resources to reduce "
            "their transfer size by 8.7KiB (99% reduction).\n  "
            "Compressing http://www.test.com/ could save 8.7KiB "
            "(99% reduction).\n", FormatResults());
}

TEST_F(EnableGzipCompressionTest, FormatNoResults) {
  Freeze();
  ASSERT_TRUE(AppendResults());
  ASSERT_EQ(
      "You have compression enabled. Learn more about enabling compression"
      "<https://developers.google.com/speed/docs/insights/"
      "EnableCompression>.\n",
      FormatResults());
}

}  // namespace
