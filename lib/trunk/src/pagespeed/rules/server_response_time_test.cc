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
    Resource* resource = NewResource(url, 200);
    resource->SetFirstByteMillis(first_byte_millis);
    resource->SetResponseBody(body);
  }

  void AddPrimaryTestResource(const std::string &url,
                              const int32 first_byte_millis,
                              const std::string &body) {
    Resource* resource = NewPrimaryResource(url);
    resource->SetFirstByteMillis(first_byte_millis);
    resource->SetResponseBody(body);
  }

  void AddRedirectTestResource(const std::string &url,
                               const std::string& location,
                               const int32 first_byte_millis) {
    Resource* resource = NewResource(url, 302);
    resource->SetFirstByteMillis(first_byte_millis);
    resource->AddResponseHeader("Location", location);
  }
};

TEST_F(ServerResponseTimeTest, FastResult) {
  AddPrimaryTestResource("http://www.example.com/hello.html",
                         1,
                         "Hello, World!");
  CheckNoViolations();
}

TEST_F(ServerResponseTimeTest, BarelyFastResult) {
  AddPrimaryTestResource("http://www.example.com/hello.html",
                         kFirstByteMillisThreshold - 1,
                         "Hello, World!");
  CheckNoViolations();
}

TEST_F(ServerResponseTimeTest, BarelySlowResult) {
  AddPrimaryTestResource("http://www.example.com/hello.html",
                         kFirstByteMillisThreshold,
                         "Hello, World!");
  CheckOneUrlViolation("http://www.example.com/hello.html");
}

TEST_F(ServerResponseTimeTest, SlowResult) {
  AddPrimaryTestResource("http://www.example.com/hello.html",
                         kFirstByteMillisThreshold * 10,
                         "Hello, World!");
  CheckOneUrlViolation("http://www.example.com/hello.html");
}

TEST_F(ServerResponseTimeTest, SlowSecondResult) {
  AddPrimaryTestResource("http://www.example.com/hello.html",
                         2,
                         "Hello, World!");
  AddTestResource("http://www.example.com/hello2.html",
                         kFirstByteMillisThreshold * 10,
                         "Hello, World!");
  CheckOneUrlViolation("http://www.example.com/hello2.html");
}

TEST_F(ServerResponseTimeTest, TwoSlowResults) {
  AddPrimaryTestResource("http://www.example.com/hello.html",
                         kFirstByteMillisThreshold * 10,
                         "Hello, World!");
  AddTestResource("http://www.example.com/hello2.html",
                         kFirstByteMillisThreshold * 10,
                         "Hello, World!");
  CheckTwoUrlViolations(
      "http://www.example.com/hello.html",
      "http://www.example.com/hello2.html");
}

TEST_F(ServerResponseTimeTest, SlowRedirect) {
  AddPrimaryTestResource("http://www.example.com/hello.html",
                         kFirstByteMillisThreshold - 1,
                         "Hello, World!");
  AddRedirectTestResource("http://www.example.com/hello2.html",
                          "http://www.example.com/hello.html",
                          kFirstByteMillisThreshold * 10);

  CheckOneUrlViolation("http://www.example.com/hello2.html");
}

TEST_F(ServerResponseTimeTest, TwoSlowRedirects) {
  AddPrimaryTestResource("http://www.example.com/hello.html",
                         kFirstByteMillisThreshold - 1,
                         "Hello, World!");

  AddRedirectTestResource("http://www.example.com/hello2.html",
                          "http://www.example.com/hello.html",
                          kFirstByteMillisThreshold * 10);

  AddRedirectTestResource("http://www.example.com/hello3.html",
                          "http://www.example.com/hello2.html",
                          kFirstByteMillisThreshold * 10);

  CheckTwoUrlViolations(
      "http://www.example.com/hello2.html",
      "http://www.example.com/hello3.html");
}

TEST_F(ServerResponseTimeTest, SlowRedirectToSlowPage) {
  AddPrimaryTestResource("http://www.example.com/hello.html",
                         kFirstByteMillisThreshold * 10,
                         "Hello, World!");

  AddRedirectTestResource("http://www.example.com/hello2.html",
                          "http://www.example.com/hello.html",
                          kFirstByteMillisThreshold * 10);

  CheckTwoUrlViolations(
      "http://www.example.com/hello.html",
      "http://www.example.com/hello2.html");
}

}  // namespace
