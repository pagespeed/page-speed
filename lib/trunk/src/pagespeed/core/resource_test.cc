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

#include <vector>

#include "base/scoped_ptr.h"
#include "pagespeed/core/javascript_call_info.h"
#include "pagespeed/core/resource.h"
#include "testing/gtest/include/gtest/gtest.h"

using pagespeed::JavaScriptCallInfo;
using pagespeed::Resource;

namespace {

// Verify that builder setters and resource getters work.
TEST(ResourceTest, SetFields) {
  Resource resource;
  resource.SetRequestUrl("http://www.test.com/");
  resource.SetRequestMethod("GET");
  resource.SetRequestBody("request body");
  resource.SetResponseStatusCode(200);
  resource.SetResponseBody("response body");

  EXPECT_EQ(resource.GetRequestUrl(), "http://www.test.com/");
  EXPECT_EQ(resource.GetRequestMethod(), "GET");
  EXPECT_EQ(resource.GetRequestBody(), "request body");
  EXPECT_EQ(resource.GetResponseStatusCode(), 200);
  EXPECT_EQ(resource.GetResponseBody(), "response body");

  EXPECT_EQ(resource.IsLazyLoaded(), false);
  resource.SetLazyLoaded();
  EXPECT_EQ(resource.IsLazyLoaded(), true);
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

TEST(ResourceTest, Cookies) {
  Resource resource;
  EXPECT_EQ("", resource.GetCookies());

  resource.AddResponseHeader("Set-Cookie", "chocolate");
  EXPECT_EQ("chocolate", resource.GetCookies());

  // The Cookie header should take precedence over the Set-Cookie
  // header.
  resource.AddRequestHeader("Cookie", "oatmeal");
  EXPECT_EQ("oatmeal", resource.GetCookies());

  // SetCookies should take precedence over the Cookie header.
  resource.SetCookies("foo");
  EXPECT_EQ("foo", resource.GetCookies());
}

void ExpectResourceType(const char *content_type,
                        int status_code,
                        pagespeed::ResourceType type) {
  Resource r;
  r.AddResponseHeader("Content-Type", content_type);
  r.SetResponseStatusCode(status_code);
  EXPECT_EQ(type, r.GetResourceType());
}

TEST(ResourceTest, ResourceTypes) {
  ExpectResourceType("text/html", 200, pagespeed::HTML);
  ExpectResourceType("text/html-sandboxed", 200, pagespeed::HTML);
  ExpectResourceType("text/html; charset=UTF-8", 200, pagespeed::HTML);
  ExpectResourceType("application/xhtml+xml", 200, pagespeed::HTML);
  ExpectResourceType("text/css", 200, pagespeed::CSS);

  // Types from
  // http://dev.w3.org/html5/spec/Overview.html#scriptingLanguages
  ExpectResourceType("application/ecmascript", 200, pagespeed::JS);
  ExpectResourceType("application/javascript", 200, pagespeed::JS);
  ExpectResourceType("application/x-ecmascript", 200, pagespeed::JS);
  ExpectResourceType("application/x-javascript", 200, pagespeed::JS);
  ExpectResourceType("text/ecmascript", 200, pagespeed::JS);
  ExpectResourceType("text/javascript", 200, pagespeed::JS);
  ExpectResourceType("text/javascript1.0", 200, pagespeed::JS);
  ExpectResourceType("text/javascript1.1", 200, pagespeed::JS);
  ExpectResourceType("text/javascript1.2", 200, pagespeed::JS);
  ExpectResourceType("text/javascript1.3", 200, pagespeed::JS);
  ExpectResourceType("text/javascript1.4", 200, pagespeed::JS);
  ExpectResourceType("text/javascript1.5", 200, pagespeed::JS);
  ExpectResourceType("text/jscript", 200, pagespeed::JS);
  ExpectResourceType("text/livescript", 200, pagespeed::JS);
  ExpectResourceType("text/x-ecmascript", 200, pagespeed::JS);
  ExpectResourceType("text/x-javascript", 200, pagespeed::JS);
  ExpectResourceType("text/javascript;e4x=1", 200, pagespeed::JS);

  // Other common JS types
  ExpectResourceType("text/json", 200, pagespeed::JS);
  ExpectResourceType("text/x-js", 200, pagespeed::JS);
  ExpectResourceType("text/x-json", 200, pagespeed::JS);
  ExpectResourceType("application/json", 200, pagespeed::JS);
  ExpectResourceType("application/x-js", 200, pagespeed::JS);
  ExpectResourceType("application/x-json", 200, pagespeed::JS);

  ExpectResourceType("text/plain", 200, pagespeed::TEXT);
  ExpectResourceType("application/xml", 200, pagespeed::TEXT);
  ExpectResourceType("image/png", 200, pagespeed::IMAGE);
  ExpectResourceType("image/jpeg", 200, pagespeed::IMAGE);
  ExpectResourceType("application/x-shockwave-flash", 200, pagespeed::FLASH);
  ExpectResourceType("application/x-binary", 200, pagespeed::OTHER);
  ExpectResourceType("text/html", 302, pagespeed::REDIRECT);
  ExpectResourceType("text/html", 100, pagespeed::OTHER);
  ExpectResourceType("text/html", 304, pagespeed::HTML);
  ExpectResourceType("text/html", 401, pagespeed::OTHER);
}

void ExpectImageType(const char *content_type,
                     int status_code,
                     pagespeed::ImageType type) {
  Resource r;
  r.SetResponseStatusCode(status_code);
  r.AddResponseHeader("Content-Type", content_type);
  EXPECT_EQ(type, r.GetImageType());
}

TEST(ResourceTest, ImageTypes) {
  ExpectImageType("image/gif", 200, pagespeed::GIF);
  ExpectImageType("image/png", 200, pagespeed::PNG);
  ExpectImageType("image/jpg", 200, pagespeed::JPEG);
  ExpectImageType("image/jpeg", 200, pagespeed::JPEG);
  ExpectImageType("image/xyz", 200, pagespeed::UNKNOWN_IMAGE_TYPE);
#ifndef NDEBUG
  EXPECT_DEATH(
      ExpectImageType("image/png", 302, pagespeed::UNKNOWN_IMAGE_TYPE),
      "Non-image type: 5");
#else
  ExpectImageType("image/png", 302, pagespeed::UNKNOWN_IMAGE_TYPE);
#endif
  ExpectImageType("image/png", 304, pagespeed::PNG);
}

TEST(ResourceTest, SetResourceTypeForRedirectFails) {
  Resource r;
  r.SetResponseStatusCode(302);
#ifdef NDEBUG
  r.SetResourceType(pagespeed::HTML);
  ASSERT_EQ(pagespeed::REDIRECT, r.GetResourceType());
#else
  ASSERT_DEATH(r.SetResourceType(pagespeed::HTML),
               "Unable to SetResourceType for redirect.");
#endif
}

TEST(ResourceTest, SetResourceTypeNoStatusCodeFails) {
  Resource r;
  r.SetResourceType(pagespeed::HTML);
  ASSERT_EQ(pagespeed::OTHER, r.GetResourceType());
}

TEST(ResourceTest, SetResourceTypeFor500Fails) {
  Resource r;
  r.SetResponseStatusCode(500);
  r.SetResourceType(pagespeed::HTML);
  ASSERT_EQ(pagespeed::OTHER, r.GetResourceType());
}

TEST(ResourceTest, SetResourceTypeToRedirectFails) {
  Resource r;
  r.SetResponseStatusCode(200);
#ifdef NDEBUG
  r.SetResourceType(pagespeed::REDIRECT);
  ASSERT_EQ(pagespeed::OTHER, r.GetResourceType());
#else
  ASSERT_DEATH(r.SetResourceType(pagespeed::REDIRECT),
               "Unable to SetResourceType to redirect.");
#endif
}

TEST(ResourceTest, SetResourceType) {
  Resource r;
  r.SetResponseStatusCode(200);
  ASSERT_EQ(pagespeed::OTHER, r.GetResourceType());
  r.AddResponseHeader("Content-Type", "text/css");
  ASSERT_EQ(pagespeed::CSS, r.GetResourceType());
  r.SetResourceType(pagespeed::HTML);
  ASSERT_EQ(pagespeed::HTML, r.GetResourceType());
  r.SetResponseStatusCode(500);
  ASSERT_EQ(pagespeed::OTHER, r.GetResourceType());
  r.SetResponseStatusCode(302);
  ASSERT_EQ(pagespeed::REDIRECT, r.GetResourceType());
}

TEST(ResourceTest, JavaScriptCallInfo) {
  const char* kDocUrl = "http://www.example.com/";
  Resource r;
  ASSERT_TRUE(NULL == r.GetJavaScriptCalls("document.write"));

  std::vector<std::string> args;
  args.push_back("<script src='foo.js'></script>");
  const JavaScriptCallInfo* info =
      new JavaScriptCallInfo("document.write", kDocUrl, args, 1);
  r.AddJavaScriptCall(info);
  const std::vector<const JavaScriptCallInfo*>* calls =
      r.GetJavaScriptCalls("document.write");
  ASSERT_TRUE(NULL != calls);
  ASSERT_EQ(1U, calls->size());

  info = new JavaScriptCallInfo("document.write", kDocUrl, args, 2);
  r.AddJavaScriptCall(info);

  info = new JavaScriptCallInfo("eval", kDocUrl, args, 3);
  r.AddJavaScriptCall(info);

  calls = r.GetJavaScriptCalls("document.write");
  ASSERT_TRUE(NULL != calls);
  ASSERT_EQ(2U, calls->size());

  info = (*calls)[0];
  ASSERT_EQ("document.write", info->id());
  ASSERT_EQ(kDocUrl, info->document_url());
  ASSERT_EQ(1U, info->args().size());
  ASSERT_EQ("<script src='foo.js'></script>", info->args()[0]);
  ASSERT_EQ(1, info->line_number());

  info = (*calls)[1];
  ASSERT_EQ("document.write", info->id());
  ASSERT_EQ(kDocUrl, info->document_url());
  ASSERT_EQ(1U, info->args().size());
  ASSERT_EQ("<script src='foo.js'></script>", info->args()[0]);
  ASSERT_EQ(2, info->line_number());

  calls = r.GetJavaScriptCalls("eval");
  ASSERT_TRUE(NULL != calls);
  ASSERT_EQ(1U, calls->size());

  info = (*calls)[0];
  ASSERT_EQ("eval", info->id());
  ASSERT_EQ(kDocUrl, info->document_url());
  ASSERT_EQ(1U, info->args().size());
  ASSERT_EQ("<script src='foo.js'></script>", info->args()[0]);
  ASSERT_EQ(3, info->line_number());

}

}  // namespace
