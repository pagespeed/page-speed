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

#include "base/scoped_ptr.h"
#include "pagespeed/core/pagespeed_input.h"
#include "pagespeed/core/resource.h"
#include "pagespeed/core/resource_filter.h"
#include "pagespeed/har/http_archive.h"
#include "testing/gtest/include/gtest/gtest.h"

using pagespeed::AllowAllResourceFilter;
using pagespeed::PagespeedInput;
using pagespeed::ParseHttpArchive;
using pagespeed::ParseHttpArchiveWithFilter;
using pagespeed::Resource;

namespace {

const char* kHarInput = (
    "{"
    "  \"log\":{"
    "    \"version\":\"1.2\","
    "    \"creator\":{\"name\":\"http_archive_test\", \"version\":\"1.0\"},"
    "    \"pages\":["
    "      {"
    "        \"startedDateTime\": \"2009-04-16T12:07:23.321Z\","
    "        \"id\": \"page_0\","
    "        \"title\": \"Example Page\","
    "        \"pageTimings\": {"
    "          \"onLoad\": 1500"
    "        }"
    "      }"
    "    ],"
    "    \"entries\":["
    "      {"
    "        \"pageref\": \"page_0\","
    "        \"startedDateTime\": \"2009-04-16T12:07:23.596Z\","
    "        \"request\":{"
    "          \"method\":\"GET\","
    "          \"url\":\"http://www.example.com/index.html\","
    "          \"httpVersion\":\"HTTP/1.1\","
    "          \"cookies\":[],"
    "          \"headers\":["
    "            {\"name\":\"X-Foo\", \"value\":\"bar\"}"
    "          ],"
    "          \"headersSize\":-1,"
    "          \"bodySize\":0"
    "        },"
    "        \"response\":{"
    "          \"status\":200,"
    "          \"statusText\":\"OK\","
    "          \"httpVersion\":\"HTTP/1.1\","
    "          \"cookies\":[],"
    "          \"headers\":["
    "            {\"name\":\"Content-Type\", \"value\":\"text/html\"}"
    "          ],"
    "          \"content\":{"
    "            \"size\":13,"
    "            \"mimeType\":\"text/html\","
    "            \"encoding\":\"\","
    "            \"text\":\"Hello, world!\""
    "          },"
    "          \"redirectUrl\":\"\","
    "          \"headersSize\":-1,"
    "          \"bodySize\":13"
    "        }"
    "      },"
    "      {"
    "        \"pageref\": \"page_0\","
    "        \"startedDateTime\": \"2009-05-16T12:07:25.596Z\","
    "        \"request\":{"
    "          \"method\":\"GET\","
    "          \"url\":\"http://www.example.com/postonload.js\","
    "          \"httpVersion\":\"HTTP/1.1\","
    "          \"cookies\":[],"
    "          \"headers\":[],"
    "          \"headersSize\":-1,"
    "          \"bodySize\":0"
    "        },"
    "        \"response\":{"
    "          \"status\":200,"
    "          \"statusText\":\"OK\","
    "          \"httpVersion\":\"HTTP/1.1\","
    "          \"cookies\":[],"
    "          \"headers\":["
    "            {\"name\":\"Content-Type\","
    "             \"value\":\"application/javascript\"}"
    "          ],"
    "          \"content\":{"
    "            \"size\":13,"
    "            \"mimeType\":\"application/javascript\","
    "            \"text\":\"Hello, world!\""
    "          },"
    "          \"redirectUrl\":\"\","
    "          \"headersSize\":-1,"
    "          \"bodySize\":13"
    "        }"
    "      }"
    "    ]"
    "  }"
    "}");

const char* kHarInputBase64 = (
    "{"
    "  \"log\":{"
    "    \"version\":\"1.2\","
    "    \"creator\":{\"name\":\"http_archive_test\", \"version\":\"1.0\"},"
    "    \"entries\":["
    "      {"
    "        \"request\":{"
    "          \"method\":\"GET\","
    "          \"url\":\"http://www.example.com/index.html\","
    "          \"httpVersion\":\"HTTP/1.1\","
    "          \"cookies\":[],"
    "          \"headers\":["
    "            {\"name\":\"X-Foo\", \"value\":\"bar\"}"
    "          ],"
    "          \"headersSize\":-1,"
    "          \"bodySize\":0"
    "        },"
    "        \"response\":{"
    "          \"status\":200,"
    "          \"statusText\":\"OK\","
    "          \"httpVersion\":\"HTTP/1.0\","
    "          \"cookies\":[],"
    "          \"headers\":["
    "            {\"name\":\"Content-Type\", \"value\":\"text/html\"}"
    "          ],"
    "          \"content\":{"
    "            \"size\":13,"
    "            \"mimeType\":\"text/html\","
    "            \"text\":\"SGVsbG8sIHdvcmxkIQ==\","
    "            \"encoding\":\"base64\""
    "          },"
    "          \"redirectUrl\":\"\","
    "          \"headersSize\":-1,"
    "          \"bodySize\":13"
    "        }"
    "      }"
    "    ]"
    "  }"
    "}");

TEST(HttpArchiveTest, ValidInput) {
  scoped_ptr<PagespeedInput> input(ParseHttpArchive(kHarInput));
  ASSERT_FALSE(input == NULL);
  input->Freeze();

  ASSERT_EQ(2, input->num_resources());

  const Resource& resource1 = input->GetResource(0);
  EXPECT_EQ("http://www.example.com/index.html", resource1.GetRequestUrl());
  EXPECT_EQ("GET", resource1.GetRequestMethod());
  EXPECT_EQ("bar", resource1.GetRequestHeader("x-foo"));
  EXPECT_EQ("", resource1.GetRequestBody());
  EXPECT_EQ(pagespeed::HTTP_11, resource1.GetResponseProtocol());
  EXPECT_EQ(200, resource1.GetResponseStatusCode());
  EXPECT_EQ("text/html", resource1.GetResponseHeader("content-type"));
  EXPECT_EQ("Hello, world!", resource1.GetResponseBody());
  EXPECT_FALSE(input->IsResourceLoadedAfterOnload(resource1));

  const Resource& resource2 = input->GetResource(1);
  EXPECT_EQ("http://www.example.com/postonload.js", resource2.GetRequestUrl());
  EXPECT_EQ("GET", resource2.GetRequestMethod());
  EXPECT_EQ("", resource2.GetRequestBody());
  EXPECT_EQ(pagespeed::HTTP_11, resource2.GetResponseProtocol());
  EXPECT_EQ(200, resource2.GetResponseStatusCode());
  EXPECT_EQ("application/javascript",
            resource2.GetResponseHeader("content-type"));
  EXPECT_EQ("Hello, world!", resource2.GetResponseBody());
  EXPECT_TRUE(input->IsResourceLoadedAfterOnload(resource2));

  // Make sure that request start time truncation works properly.
  Resource other;
  other.SetRequestStartTimeMillis(kint32max);
  EXPECT_FALSE(resource2.IsRequestStartTimeLessThan(other));
  EXPECT_FALSE(other.IsRequestStartTimeLessThan(resource2));

  other.SetRequestStartTimeMillis(kint32max - 1);
  EXPECT_FALSE(resource2.IsRequestStartTimeLessThan(other));
  EXPECT_TRUE(other.IsRequestStartTimeLessThan(resource2));
}

TEST(HttpArchiveTest, ValidInputBase64) {
  scoped_ptr<PagespeedInput> input(ParseHttpArchive(kHarInputBase64));
  ASSERT_FALSE(input == NULL);
  input->Freeze();

  ASSERT_EQ(1, input->num_resources());

  const Resource& resource1 = input->GetResource(0);
  EXPECT_EQ("http://www.example.com/index.html", resource1.GetRequestUrl());
  EXPECT_EQ("GET", resource1.GetRequestMethod());
  EXPECT_EQ("bar", resource1.GetRequestHeader("x-foo"));
  EXPECT_EQ("", resource1.GetRequestBody());
  EXPECT_EQ(pagespeed::HTTP_10, resource1.GetResponseProtocol());
  EXPECT_EQ(200, resource1.GetResponseStatusCode());
  EXPECT_EQ("text/html", resource1.GetResponseHeader("content-type"));
  EXPECT_EQ("Hello, world!", resource1.GetResponseBody());
  EXPECT_FALSE(input->IsResourceLoadedAfterOnload(resource1));
}

TEST(HttpArchiveTest, InvalidJSON) {
  scoped_ptr<PagespeedInput> input(ParseHttpArchive("{\"log\":}"));
  ASSERT_EQ(NULL, input.get());
}

TEST(HttpArchiveTest, InvalidJSONWithFilter) {
  scoped_ptr<PagespeedInput> input(
      ParseHttpArchiveWithFilter("{\"log\":}", new AllowAllResourceFilter()));
  ASSERT_EQ(NULL, input.get());
}

TEST(HttpArchiveTest, InvalidHAR) {
  scoped_ptr<PagespeedInput> input(ParseHttpArchive("{\"log\":[]}"));
  ASSERT_EQ(NULL, input.get());
}

TEST(HttpArchiveTest, MissingText) {
  const char* kHarMissingResourceBody =
      "{"
      "  \"log\":{"
      "    \"entries\":["
      "      {"
      "        \"startedDateTime\": \"2009-04-16T12:07:23.596Z\","
      "        \"request\":{"
      "          \"method\":\"GET\","
      "          \"url\":\"http://www.example.com/index.html\","
      "          \"httpVersion\":\"HTTP/1.1\","
      "          \"cookies\":[],"
      "          \"headers\":["
      "            {\"name\":\"X-Foo\", \"value\":\"bar\"}"
      "          ],"
      "          \"headersSize\":-1,"
      "          \"bodySize\":0"
      "        },"
      "        \"response\":{"
      "          \"status\":204,"
      "          \"statusText\":\"OK\","
      "          \"httpVersion\":\"HTTP/1.1\","
      "          \"headers\":[],"
      "          \"content\":{},"
      "        }"
      "      }"
      "    ]"
      "  }"
      "}";

  scoped_ptr<PagespeedInput> input(ParseHttpArchive(kHarMissingResourceBody));
  ASSERT_TRUE(input.get() != NULL);
}

class Iso8601Test : public testing::Test {
 protected:
  void ExpectValid(const std::string& input, int64 output) {
    int64 millis = -31337;
    EXPECT_TRUE(pagespeed::Iso8601ToEpochMillis(input, &millis));
    EXPECT_EQ(output, millis);
  }
  void ExpectInvalid(const std::string& input) {
    int64 millis = -31337;
    EXPECT_FALSE(pagespeed::Iso8601ToEpochMillis(input, &millis));
    EXPECT_EQ(-31337, millis);
  }
};

TEST_F(Iso8601Test, ValidIso8601) {
  // Check some simple cases:
  ExpectValid("1970-01-01T00:00:00Z", 0);
  ExpectValid("1970-01-01T00:01:31Z", 91000);
  ExpectValid("1970-01-01T00:01:30.000Z", 90000);
  // Check several variations on the fractional part:
  ExpectValid("1970-01-01T00:01:30.123Z", 90123);
  ExpectValid("1970-01-01T00:01:30.89Z", 90890);
  ExpectValid("1970-01-01T00:01:30.6Z", 90600);
  ExpectValid("1970-01-01T00:01:30.56712Z", 90567);
  // Try out various timezone offsets:
  ExpectValid("1970-01-02T00:00:35.8Z", 86435800);
  ExpectValid("1970-01-02T00:00:35.8+03:00", 75635800);
  ExpectValid("1970-01-02T00:00:35.8-02:41", 96095800);
  // Now try a nontrivial datetime:
  ExpectValid("2010-09-07T15:39:50.044-04:00", 1283888390044LL);
  // Make sure we handle "negative" datetimes (i.e. those before 1970):
  ExpectValid("1969-12-31T23:59:59.000Z", -1000);
  ExpectValid("1969-12-31T23:00:00Z", -3600000);
}

TEST_F(Iso8601Test, InvalidIso8601) {
  // Malformed datetimes:
  ExpectInvalid("");
  ExpectInvalid("foobar");
  ExpectInvalid("12:35:20");
  ExpectInvalid("1970-01-02 00:00:35.8Z");
  ExpectInvalid("1970-01-02T00:00:35.8Q");
  // Extra characters at the end:
  ExpectInvalid("1970-01-01T00:00:00Z+");
  ExpectInvalid("1970-01-02T00:00:35.8+03:000");
  // Fields out of range:
  ExpectInvalid("1970-00-01T00:00:00Z");
  ExpectInvalid("1970-13-01T00:00:00Z");
  ExpectInvalid("1970-01-00T00:00:00Z");
  ExpectInvalid("1970-01-40T00:00:00Z");
  ExpectInvalid("1970-01-01T25:00:00Z");
  ExpectInvalid("1970-01-01T00:75:00Z");
  ExpectInvalid("1970-01-01T00:00:77Z");
}

}  // namespace
