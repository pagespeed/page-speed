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

#include "base/scoped_ptr.h"
#include "pagespeed/core/resource.h"
#include "testing/gtest/include/gtest/gtest.h"

using pagespeed::Resource;

namespace {

// Verify that builder setters and resource getters work.
TEST(ResourceTest, SetFields) {
  Resource resource;
  resource.SetRequestUrl("http://www.test.com/");
  resource.SetRequestMethod("GET");
  resource.SetRequestProtocol("HTTP");
  resource.SetRequestBody("request body");
  resource.SetResponseStatusCode(200);
  resource.SetResponseProtocol("HTTP/1.1");
  resource.SetResponseBody("response body");

  EXPECT_EQ(resource.GetRequestUrl(), "http://www.test.com/");
  EXPECT_EQ(resource.GetRequestMethod(), "GET");
  EXPECT_EQ(resource.GetRequestProtocol(), "HTTP");
  EXPECT_EQ(resource.GetRequestBody(), "request body");
  EXPECT_EQ(resource.GetResponseStatusCode(), 200);
  EXPECT_EQ(resource.GetResponseProtocol(), "HTTP/1.1");
  EXPECT_EQ(resource.GetResponseBody(), "response body");
}

// Verify that http header matching is case-insensitive.
TEST(ResourceTest, HeaderFields) {
  Resource resource;
  resource.AddRequestHeader("request_lower", "Re 1");
  resource.AddRequestHeader("REQUEST_UPPER", "Re 2");
  resource.AddResponseHeader("response_lower", "Re 3");
  resource.AddResponseHeader("RESPONSE_UPPER", "Re 4");
  resource.AddRequestHeader("duplicate request", "1");
  resource.AddRequestHeader("Duplicate request", "2");
  resource.AddResponseHeader("duplicate response", "3");
  resource.AddResponseHeader("Duplicate response", "4");

  EXPECT_EQ(resource.GetRequestHeader("request_lower"), "Re 1");
  EXPECT_EQ(resource.GetRequestHeader("Request_Lower"), "Re 1");
  EXPECT_EQ(resource.GetRequestHeader("REQUEST_LOWER"), "Re 1");

  EXPECT_EQ(resource.GetRequestHeader("request_upper"), "Re 2");
  EXPECT_EQ(resource.GetRequestHeader("Request_Upper"), "Re 2");
  EXPECT_EQ(resource.GetRequestHeader("REQUEST_UPPER"), "Re 2");

  EXPECT_EQ(resource.GetRequestHeader("request_unknown"), "");
  EXPECT_EQ(resource.GetRequestHeader("response_lower"), "");

  EXPECT_EQ(resource.GetResponseHeader("response_lower"), "Re 3");
  EXPECT_EQ(resource.GetResponseHeader("Response_Lower"), "Re 3");
  EXPECT_EQ(resource.GetResponseHeader("RESPONSE_LOWER"), "Re 3");

  EXPECT_EQ(resource.GetResponseHeader("response_upper"), "Re 4");
  EXPECT_EQ(resource.GetResponseHeader("Response_Upper"), "Re 4");
  EXPECT_EQ(resource.GetResponseHeader("RESPONSE_UPPER"), "Re 4");

  EXPECT_EQ(resource.GetResponseHeader("response_unknown"), "");
  EXPECT_EQ(resource.GetResponseHeader("request_lower"), "");

  EXPECT_EQ(resource.GetRequestHeader("duplicate request"), "1,2");
  EXPECT_EQ(resource.GetResponseHeader("duplicate response"), "3,4");
}

void ExpectResourceType(
    const char *content_type, pagespeed::ResourceType type) {
  Resource r;
  r.AddResponseHeader("Content-Type", content_type);
  EXPECT_EQ(type, r.GetResourceType());
}

TEST(ResourceTest, ResourceTypes) {
  ExpectResourceType("text/html", pagespeed::HTML);
  ExpectResourceType("text/html; charset=UTF-8", pagespeed::HTML);
  ExpectResourceType("text/css", pagespeed::CSS);
  ExpectResourceType("text/javascript", pagespeed::JS);
  ExpectResourceType("application/x-javascript", pagespeed::JS);
  ExpectResourceType("text/plain", pagespeed::TEXT);
  ExpectResourceType("image/png", pagespeed::IMAGE);
  ExpectResourceType("image/jpeg", pagespeed::IMAGE);
  ExpectResourceType("application/x-binary", pagespeed::OTHER);
}

}  // namespace
