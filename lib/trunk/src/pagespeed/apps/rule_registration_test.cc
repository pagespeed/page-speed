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

#include "pagespeed/core/engine.h"
#include "pagespeed/core/pagespeed_input.h"
#include "pagespeed/core/pagespeed_input.pb.h"
#include "pagespeed/core/pagespeed_options.pb.h"
#include "pagespeed/core/pagespeed_output.pb.h"
#include "pagespeed/core/proto_resource_utils.h"
#include "pagespeed/core/rule_registry.h"
#include "testing/gtest/include/gtest/gtest.h"

using pagespeed::Engine;
using pagespeed::Options;
using pagespeed::PagespeedInput;
using pagespeed::ProtoInput;
using pagespeed::ProtoResource;
using pagespeed::ProtoResourceUtils;
using pagespeed::Result;
using pagespeed::Results;
using pagespeed::RuleRegistry;

namespace {

class RuleRegistrationTest : public testing::Test {
 public:
  static void SetUpTestCase() {
    RuleRegistry::Freeze();
  }
};

// Test that verifies that the link-time dependencies on the rule
// objects causes those rules to get linked in.
TEST_F(RuleRegistrationTest, LinkTimeDependencyTest) {
  ProtoInput proto_input;

  ProtoResource* proto_resource = proto_input.add_resources();
  proto_resource->set_request_url("http://www.test.com/");
  proto_resource->set_request_method("GET");
  proto_resource->set_request_protocol("HTTP");
  proto_resource->set_response_status_code(200);
  proto_resource->set_response_protocol("HTTP/1.1");
  ProtoResourceUtils::AddResponseHeader(
      proto_resource, "Content-Type", "text/html");
  ProtoResourceUtils::AddResponseHeader(
      proto_resource, "Content-Length", "6000");

  PagespeedInput input(&proto_input);

  Options options;
  options.add_rule_names("GzipRule");

  Engine engine;
  Results results;
  engine.GetResults(input, options, &results);
  ASSERT_EQ(results.results_size(), 1);

  const Result& result = results.results(0);
  EXPECT_EQ(result.rule_name(), "GzipRule");
}

// Test that verifies the behavior of passing in an empty options object.
TEST_F(RuleRegistrationTest, EmptyOptionsTest) {
  ProtoInput proto_input;

  ProtoResource* proto_resource = proto_input.add_resources();
  proto_resource->set_request_url("http://www.test.com/");
  proto_resource->set_request_method("GET");
  proto_resource->set_request_protocol("HTTP");
  proto_resource->set_response_status_code(200);
  proto_resource->set_response_protocol("HTTP/1.1");
  ProtoResourceUtils::AddResponseHeader(
      proto_resource, "Content-Type", "text/html");
  ProtoResourceUtils::AddResponseHeader(
      proto_resource, "Content-Length", "6000");

  PagespeedInput input(&proto_input);

  Options options;

  Engine engine;
  Results results;
  engine.GetResults(input, options, &results);
  // Expect 1 result per registered rule.  We expect the gzip rule and
  // at least 1 other rule to be registered.
  ASSERT_GT(results.results_size(), 1);
}

}  // namespace
