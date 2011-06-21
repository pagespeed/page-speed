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

// Author: Matthew Steele

#include "pagespeed/core/resource.h"
#include "testing/gtest/include/gtest/gtest.h"

#include "pagespeed_firefox/cpp/pagespeed/pagespeed_json_input.h"

using namespace pagespeed;

namespace {

TEST(PagespeedJsonInputTest, Empty) {
  PagespeedInput input;
  const char *data = "[]";
  const bool ok = PopulateInputFromJSON(&input, data);
  ASSERT_TRUE(ok);
  ASSERT_EQ(0, input.num_resources());
}

TEST(PagespeedJsonInputTest, OneResource) {
  PagespeedInput input;
  const char *data = ("[{"
                      "\"url\":\"http://www.example.com/foo\","
                      "\"cookieString\":\"cookiecookiecookie\""
                      "}]");
  Resource* r = new Resource();
  r->SetRequestUrl("http://www.example.com/foo");
  r->SetResponseStatusCode(200);
  ASSERT_TRUE(input.AddResource(r));
  const bool ok = PopulateInputFromJSON(&input, data);
  ASSERT_TRUE(ok);
  ASSERT_EQ(1, input.num_resources());
  const Resource &resource = input.GetResource(0);
  ASSERT_EQ("http://www.example.com/foo", resource.GetRequestUrl());
  ASSERT_EQ("cookiecookiecookie", resource.GetCookies());
}

TEST(PagespeedJsonInputTest, TwoResources) {
  PagespeedInput input;
  Resource* r = new Resource();
  r->SetRequestUrl("http://www.example.com/foo");
  r->SetResponseStatusCode(200);
  ASSERT_TRUE(input.AddResource(r));
  r = new Resource();
  r->SetRequestUrl("http://www.example.com/bar");
  r->SetResponseStatusCode(200);
  ASSERT_TRUE(input.AddResource(r));
  const char *data = ("[{\"url\":\"http://www.example.com/foo\","
                      "\"cookieString\":\"cookiecookiecookie\""
                      "},"
                      "{\"url\":\"http://www.example.com/bar\","
                      "\"cookieString\":\"morecookies\""
                      "}]");
  const bool ok = PopulateInputFromJSON(&input, data);
  ASSERT_TRUE(ok);
  ASSERT_EQ(2, input.num_resources());
  const Resource &resource1 = input.GetResource(0);
  ASSERT_EQ("http://www.example.com/foo", resource1.GetRequestUrl());
  ASSERT_EQ("cookiecookiecookie", resource1.GetCookies());
  const Resource &resource2 = input.GetResource(1);
  ASSERT_EQ("http://www.example.com/bar", resource2.GetRequestUrl());
  ASSERT_EQ("morecookies", resource2.GetCookies());
}

TEST(PagespeedJsonErrorHandlingTest, Garbage) {
  PagespeedInput input;
  const char *data = "]{!#&$*@";
#ifdef NDEBUG
  const bool ok = PopulateInputFromJSON(&input, data);
  ASSERT_FALSE(ok);
#else
  ASSERT_DEATH(PopulateInputFromJSON(&input, data),
               "Input was not valid JSON.");
#endif
}

TEST(PagespeedJsonErrorHandlingTest, InvalidKey) {
  PagespeedInput input;
  Resource* r = new Resource();
  r->SetRequestUrl("http://www.example.com/foo");
  r->SetResponseStatusCode(200);
  ASSERT_TRUE(input.AddResource(r));
  const char *data = ("[{\"url\":\"http://www.example.com/foo\","
                        "\"the_answer\":42}]");
#ifdef NDEBUG
  const bool ok = PopulateInputFromJSON(&input, data);
  ASSERT_FALSE(ok);
#else
  ASSERT_DEATH(PopulateInputFromJSON(&input, data),
               "Unknown attribute key: the_answer");
#endif
}

}  // namespace
