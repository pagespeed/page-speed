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
#include "pagespeed/rules/server_response_time.h"
#include "pagespeed/testing/pagespeed_test.h"

using pagespeed::rules::ServerResponseTime;
using pagespeed::PagespeedInput;
using pagespeed::Resource;
using pagespeed::Result;
using pagespeed::Results;
using pagespeed::ResultProvider;
using pagespeed_testing::PagespeedRuleTest;

namespace {

const int kFirstByteMillisThreshold = 100;


class ServerResponseTimeTest : public PagespeedRuleTest<ServerResponseTime> {
 protected:
  void AddTestResource(const std::string &url,
                       const int32 first_byte_millis,
                       const std::string &body) {
    Resource* resource = new Resource;
    resource->SetResponseStatusCode(200);
    resource->SetRequestUrl(url);
    resource->SetFirstByteMillis(first_byte_millis);
    resource->SetResponseBody(body);
    AddResource(resource);
  }
};

TEST_F(ServerResponseTimeTest, FastResult) {
  AddTestResource("http://www.example.com/hello.html",
                  1,
                  "Hello, World!");
  CheckNoViolations();
}

TEST_F(ServerResponseTimeTest, BarelyFastResult) {
  AddTestResource("http://www.example.com/hello.html",
                  kFirstByteMillisThreshold - 1,
                  "Hello, World!");
  CheckNoViolations();
}

TEST_F(ServerResponseTimeTest, BarelySlowResult) {
  AddTestResource("http://www.example.com/hello.html",
                  kFirstByteMillisThreshold,
                  "Hello, World!");
  CheckOneUrlViolation("http://www.example.com/hello.html");
}

TEST_F(ServerResponseTimeTest, SlowResult) {
  AddTestResource("http://www.example.com/hello.html",
                  kFirstByteMillisThreshold * 10,
                  "Hello, World!");
  CheckOneUrlViolation("http://www.example.com/hello.html");
}

TEST_F(ServerResponseTimeTest, SlowSecondResult) {
  AddTestResource("http://www.example.com/hello.html",
                  2,
                  "Hello, World!");
  AddTestResource("http://www.example.com/hello2.html",
                  kFirstByteMillisThreshold * 10,
                  "Hello, World!");
  CheckOneUrlViolation("http://www.example.com/hello2.html");
}

TEST_F(ServerResponseTimeTest, TwoSlowResults) {
  AddTestResource("http://www.example.com/hello.html",
                  kFirstByteMillisThreshold * 10,
                  "Hello, World!");
  AddTestResource("http://www.example.com/hello2.html",
                  kFirstByteMillisThreshold * 10,
                  "Hello, World!");
  CheckTwoUrlViolations(
      "http://www.example.com/hello.html",
      "http://www.example.com/hello2.html");
}


}  // namespace
