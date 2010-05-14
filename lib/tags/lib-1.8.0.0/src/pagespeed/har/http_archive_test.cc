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
#include "pagespeed/har/http_archive.h"
#include "testing/gtest/include/gtest/gtest.h"

using pagespeed::PagespeedInput;
using pagespeed::ParseHttpArchive;
using pagespeed::Resource;

namespace {

const std::string kHarInput = (
    "{"
    "  \"log\":{"
    "    \"version\":\"1.1\","
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
    "          \"httpVersion\":\"HTTP/1.1\","
    "          \"cookies\":[],"
    "          \"headers\":["
    "            {\"name\":\"Content-Type\", \"value\":\"text/html\"}"
    "          ],"
    "          \"content\":{"
    "            \"size\":13,"
    "            \"mimeType\":\"text/html\","
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

TEST(HttpArchiveTest, ValidInput) {
  scoped_ptr<PagespeedInput> input(ParseHttpArchive(kHarInput));
  ASSERT_TRUE(input.get());

  ASSERT_EQ(1, input->num_resources());
  const Resource& resource = input->GetResource(0);

  ASSERT_EQ("http://www.example.com/index.html", resource.GetRequestUrl());
  ASSERT_EQ("GET", resource.GetRequestMethod());
  ASSERT_EQ("HTTP/1.1", resource.GetRequestProtocol());
  ASSERT_EQ("bar", resource.GetRequestHeader("x-foo"));
  ASSERT_EQ("", resource.GetRequestBody());

  ASSERT_EQ(200, resource.GetResponseStatusCode());
  ASSERT_EQ("HTTP/1.1", resource.GetResponseProtocol());
  ASSERT_EQ("text/html", resource.GetResponseHeader("content-type"));
  ASSERT_EQ("Hello, world!", resource.GetResponseBody());

  ASSERT_FALSE(resource.IsLazyLoaded());
}

TEST(HttpArchiveTest, InvalidJSON) {
  scoped_ptr<PagespeedInput> input(ParseHttpArchive("{\"log\":}"));
  ASSERT_EQ(NULL, input.get());
}

TEST(HttpArchiveTest, InvalidHAR) {
  scoped_ptr<PagespeedInput> input(ParseHttpArchive("{\"log\":[]}"));
  ASSERT_EQ(NULL, input.get());
}

}  // namespace
