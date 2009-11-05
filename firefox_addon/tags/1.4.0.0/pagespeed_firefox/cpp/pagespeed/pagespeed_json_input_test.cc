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
  PagespeedInput input;
  const char *data = "[]";
  const bool ok = PopulateInputFromJSON(&input, data);
  ASSERT_TRUE(ok);
  ASSERT_EQ(0, input.num_resources());
}

TEST(PagespeedJsonInputTest, OneResource) {
  PagespeedInput input;
  const char *data = ("[{"
                      "\"req_url\":\"http://www.example.com/foo\","
                      "\"req_method\":\"GET\","
                      "\"req_protocol\":\"http\","
                      "\"req_headers\":[],"
                      "\"res_status\":200,"
                      "\"res_protocol\":\"http\","
                      "\"res_headers\":[]"
                      "}]");
  const bool ok = PopulateInputFromJSON(&input, data);
  ASSERT_TRUE(ok);
  ASSERT_EQ(1, input.num_resources());
  const Resource &resource = input.GetResource(0);
  ASSERT_EQ("http://www.example.com/foo", resource.GetRequestUrl());
  ASSERT_EQ("GET", resource.GetRequestMethod());
  ASSERT_EQ("http", resource.GetRequestProtocol());
  ASSERT_EQ(0, resource.GetRequestHeaders()->size());
  ASSERT_EQ(200, resource.GetResponseStatusCode());
  ASSERT_EQ("http", resource.GetResponseProtocol());
  ASSERT_EQ(0, resource.GetResponseHeaders()->size());
}

TEST(PagespeedJsonInputTest, TwoResources) {
  PagespeedInput input;
  const char *data = ("[{\"req_url\":\"http://www.example.com/foo\"},"
                       "{\"req_url\":\"http://www.example.com/bar\"}]");
  const bool ok = PopulateInputFromJSON(&input, data);
  ASSERT_TRUE(ok);
  ASSERT_EQ(2, input.num_resources());
}

TEST(PagespeedJsonErrorHandlingTest, Garbage) {
  PagespeedInput input;
  const char *data = "]{!#&$*@";
  const bool ok = PopulateInputFromJSON(&input, data);
  ASSERT_FALSE(ok);
}

TEST(PagespeedJsonErrorHandlingTest, InvalidKey) {
  PagespeedInput input;
  const char *data = ("[{\"req_url\":\"http://www.example.com/foo\","
                         "\"the_answer\":42}]");
  const bool ok = PopulateInputFromJSON(&input, data);
  ASSERT_FALSE(ok);
}

TEST(PagespeedJsonErrorHandlingTest, InvalidType) {
  PagespeedInput input;
  const char *data = ("[{\"req_url\":\"http://www.example.com/foo\","
                         "\"req_method\":42}]");
  const bool ok = PopulateInputFromJSON(&input, data);
  ASSERT_FALSE(ok);
}

}  // namespace
