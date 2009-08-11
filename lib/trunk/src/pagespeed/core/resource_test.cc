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
#include "pagespeed/core/pagespeed_input.pb.h"
#include "pagespeed/core/proto_resource_utils.h"
#include "pagespeed/core/resource.h"
#include "testing/gtest/include/gtest/gtest.h"

using pagespeed::ProtoResource;
using pagespeed::ProtoResourceUtils;
using pagespeed::Resource;

namespace {

// Verify that builder setters and resource getters work.
TEST(ResourceTest, SetFields) {
  ProtoResource proto_resource;
  proto_resource.set_request_url("http://www.test.com/");
  proto_resource.set_request_method("GET");
  proto_resource.set_request_protocol("HTTP");
  proto_resource.set_request_body("request body");
  proto_resource.set_response_status_code(200);
  proto_resource.set_response_protocol("HTTP/1.1");
  proto_resource.set_response_body("response body");

  const Resource resource(proto_resource);

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
  ProtoResource proto_resource;
  ProtoResourceUtils::AddRequestHeader(
      &proto_resource, "request_lower", "Re 1");
  ProtoResourceUtils::AddRequestHeader(
      &proto_resource, "REQUEST_UPPER", "Re 2");
  ProtoResourceUtils::AddResponseHeader(
      &proto_resource, "response_lower", "Re 3");
  ProtoResourceUtils::AddResponseHeader(
      &proto_resource, "RESPONSE_UPPER", "Re 4");
  ProtoResourceUtils::AddRequestHeader(
      &proto_resource, "duplicate request", "1");
  ProtoResourceUtils::AddRequestHeader(
      &proto_resource, "Duplicate request", "2");
  ProtoResourceUtils::AddResponseHeader(
      &proto_resource, "duplicate response", "3");
  ProtoResourceUtils::AddResponseHeader(
      &proto_resource, "Duplicate response", "4");

  const Resource resource(proto_resource);

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

}  // namespace
