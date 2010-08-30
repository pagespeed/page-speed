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

#include "pagespeed_json_input.h"

using namespace pagespeed;

namespace {

TEST(PagespeedJsonInputTest, Empty) {
  std::vector<std::string> contents;
  PagespeedInput input;
  const char *data = "[]";
  const bool ok = PopulateInputFromJSON(&input, data, contents);
  ASSERT_TRUE(ok);
  ASSERT_EQ(0, input.num_resources());
}

TEST(PagespeedJsonInputTest, OneResource) {
  std::vector<std::string> contents;
  PagespeedInput input;
  const char *data = ("[{"
                      "\"req_url\":\"http://www.example.com/foo\","
                      "\"req_method\":\"GET\","
                      "\"req_headers\":[],"
                      "\"res_status\":200,"
                      "\"res_headers\":[],"
                      "\"req_lazy_loaded\":true"
                      "}]");
  const bool ok = PopulateInputFromJSON(&input, data, contents);
  ASSERT_TRUE(ok);
  ASSERT_EQ(1, input.num_resources());
  const Resource &resource = input.GetResource(0);
  ASSERT_EQ("http://www.example.com/foo", resource.GetRequestUrl());
  ASSERT_EQ("GET", resource.GetRequestMethod());
  ASSERT_EQ(0, resource.GetRequestHeaders()->size());
  ASSERT_EQ(200, resource.GetResponseStatusCode());
  ASSERT_EQ(0, resource.GetResponseHeaders()->size());
  ASSERT_EQ(true, resource.IsLazyLoaded());
}

TEST(PagespeedJsonInputTest, ResourceLazyLoaded) {
  std::vector<std::string> contents;
  PagespeedInput input;
  const char *data = ("[{\"req_url\":\"http://www.example.com/foo\","
                      "\"res_status\":200,"
                      "\"req_lazy_loaded\":false"
                      "},"
                      "{\"req_url\":\"http://www.example.com/goo\","
                      "\"res_status\":200"
                      "},"
                      "{\"req_url\":\"http://www.example.com/bar\","
                      "\"res_status\":200,"
                      "\"req_lazy_loaded\":true"
                      "}]");
  const bool ok = PopulateInputFromJSON(&input, data, contents);
  ASSERT_TRUE(ok);
  ASSERT_EQ(3, input.num_resources());
  const Resource &resource1 = input.GetResource(0);
  ASSERT_EQ("http://www.example.com/foo", resource1.GetRequestUrl());
  ASSERT_EQ(false, resource1.IsLazyLoaded());
  const Resource &resource2 = input.GetResource(1);
  ASSERT_EQ("http://www.example.com/goo", resource2.GetRequestUrl());
  ASSERT_EQ(false, resource2.IsLazyLoaded());
  const Resource &resource3 = input.GetResource(2);
  ASSERT_EQ("http://www.example.com/bar", resource3.GetRequestUrl());
  ASSERT_EQ(true, resource3.IsLazyLoaded());
}

TEST(PagespeedJsonInputTest, TwoResources) {
  std::vector<std::string> contents;
  PagespeedInput input;
  const char *data = ("[{\"req_url\":\"http://www.example.com/foo\","
                      "\"res_status\":200"
                      "},"
                      "{\"req_url\":\"http://www.example.com/bar\","
                      "\"res_status\":200"
                      "}]");
  const bool ok = PopulateInputFromJSON(&input, data, contents);
  ASSERT_TRUE(ok);
  ASSERT_EQ(2, input.num_resources());
  const Resource &resource1 = input.GetResource(0);
  ASSERT_EQ("http://www.example.com/foo", resource1.GetRequestUrl());
  const Resource &resource2 = input.GetResource(1);
  ASSERT_EQ("http://www.example.com/bar", resource2.GetRequestUrl());
}

TEST(PagespeedJsonInputTest, BodyIndices) {
  std::vector<std::string> contents;
  contents.push_back("The quick brown fox jumped over the lazy dog.");
  contents.push_back("\xDE\xAD\xBE\xEF");
  PagespeedInput input;
  const char *data = ("[{\"req_url\":\"http://www.example.com/foo\","
                      "\"res_status\":200,"
                      "\"res_body\":1},"
                      "{\"req_url\":\"http://www.example.com/bar\","
                      "\"res_status\":200,"
                      "\"res_body\":0}]");
  const bool ok = PopulateInputFromJSON(&input, data, contents);
  ASSERT_TRUE(ok);
  ASSERT_EQ(2, input.num_resources());
  const Resource &resource1 = input.GetResource(0);
  ASSERT_EQ("http://www.example.com/foo", resource1.GetRequestUrl());
  ASSERT_EQ("\xDE\xAD\xBE\xEF", resource1.GetResponseBody());
  const Resource &resource2 = input.GetResource(1);
  ASSERT_EQ("http://www.example.com/bar", resource2.GetRequestUrl());
  ASSERT_EQ("The quick brown fox jumped over the lazy dog.",
            resource2.GetResponseBody());
}

TEST(PagespeedJsonErrorHandlingTest, Garbage) {
  std::vector<std::string> contents;
  PagespeedInput input;
  const char *data = "]{!#&$*@";
#ifdef NDEBUG
  const bool ok = PopulateInputFromJSON(&input, data, contents);
  ASSERT_FALSE(ok);
#else
  ASSERT_DEATH(PopulateInputFromJSON(&input, data, contents),
               "Input was not valid JSON.");
#endif
}

TEST(PagespeedJsonErrorHandlingTest, InvalidKey) {
  std::vector<std::string> contents;
  PagespeedInput input;
  const char *data = ("[{\"req_url\":\"http://www.example.com/foo\","
                        "\"the_answer\":42}]");
#ifdef NDEBUG
  const bool ok = PopulateInputFromJSON(&input, data, contents);
  ASSERT_FALSE(ok);
#else
  ASSERT_DEATH(PopulateInputFromJSON(&input, data, contents),
               "Unknown attribute key: the_answer");
#endif

  const char *data2 = ("[{\"req_url\":\"http://www.example.com/foo\","
                         "\"req_lazy_loaded\":1}]");
#ifdef NDEBUG
  const bool ok2 = PopulateInputFromJSON(&input, data2, contents);
  ASSERT_FALSE(ok2);
#else
  ASSERT_DEATH(PopulateInputFromJSON(&input, data2, contents),
               "req_lazy_loaded: 3");
#endif

}

TEST(PagespeedJsonErrorHandlingTest, InvalidType) {
  std::vector<std::string> contents;
  PagespeedInput input;
  const char *data = ("[{\"req_url\":\"http://www.example.com/foo\","
                        "\"req_method\":42}]");
#ifdef NDEBUG
  const bool ok = PopulateInputFromJSON(&input, data, contents);
  ASSERT_FALSE(ok);
#else
  ASSERT_DEATH(PopulateInputFromJSON(&input, data, contents),
               "Expected string value.");
#endif
}

TEST(PagespeedJsonErrorHandlingTest, InvalidBodyIndex) {
  std::vector<std::string> contents;
  contents.push_back("The quick brown fox jumped over the lazy dog.");
  PagespeedInput input;
  const char *data = ("[{\"req_url\":\"http://www.example.com/foo\","
                        "\"res_body\":1}]");
#ifdef NDEBUG
  const bool ok = PopulateInputFromJSON(&input, data, contents);
  ASSERT_FALSE(ok);
#else
  ASSERT_DEATH(PopulateInputFromJSON(&input, data, contents),
               "Body index out of range: 1");
#endif
}

}  // namespace
