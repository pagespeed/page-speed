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
#include "base/string_util.h"  // for StringPrintf
#include "pagespeed/core/pagespeed_input.h"
#include "pagespeed/core/resource.h"
#include "pagespeed/core/result_provider.h"
#include "pagespeed/proto/pagespeed_output.pb.h"
#include "pagespeed/rules/parallelize_downloads_across_hostnames.h"
#include "pagespeed/testing/pagespeed_test.h"

using pagespeed::rules::ParallelizeDownloadsAcrossHostnames;
using pagespeed::PagespeedInput;
using pagespeed::ParallelizableHostDetails;
using pagespeed::Resource;
using pagespeed::Result;
using pagespeed::Results;
using pagespeed::ResultProvider;
using pagespeed_testing::PagespeedRuleTest;

namespace {

class ParallelizeDownloadsAcrossHostnamesTest
    : public PagespeedRuleTest<ParallelizeDownloadsAcrossHostnames> {
 protected:
  void AddStaticResources(int num, const std::string& host) {
    for (int index = 0; index < num; ++index) {
      Resource* resource = new Resource;
      resource->SetRequestUrl(StringPrintf("http://%s/resource%d.css",
                                           host.c_str(), index));
      resource->SetRequestMethod("GET");
      resource->SetResponseStatusCode(200);
      resource->AddResponseHeader("Content-Type", "text/css");
      resource->SetResponseBody("Hello, world!");
      AddResource(resource);
    }
  }

  void CheckNoViolations() {
    Freeze();
    ASSERT_TRUE(AppendResults());
    ASSERT_EQ(0, num_results());
  }

  void CheckOneViolation(const std::string &host,
                         int critical_path_saved) {
    Freeze();
    ASSERT_TRUE(AppendResults());
    ASSERT_EQ(1, num_results());
    const Result& result0 = result(0);
    ASSERT_EQ(host, result0.details().GetExtension(
        ParallelizableHostDetails::message_set_extension).host());
    ASSERT_EQ(critical_path_saved,
              result0.savings().critical_path_length_saved());
  }
};

TEST_F(ParallelizeDownloadsAcrossHostnamesTest, NotManyResources) {
  AddStaticResources(7, "static.example.com");
  CheckNoViolations();
}

TEST_F(ParallelizeDownloadsAcrossHostnamesTest, BalancedResources) {
  AddStaticResources(51, "static1.example.com");
  AddStaticResources(52, "static2.example.com");
  AddStaticResources(55, "static3.example.com");
  AddStaticResources(53, "static4.example.com");
  CheckNoViolations();
}

TEST_F(ParallelizeDownloadsAcrossHostnamesTest, JustOneHost) {
  AddStaticResources(80, "static.example.com");
  CheckOneViolation("static.example.com", 40);
}

TEST_F(ParallelizeDownloadsAcrossHostnamesTest, UnbalancedResources) {
  AddStaticResources(10, "static1.example.com");
  AddStaticResources(30, "static2.example.com");
  CheckOneViolation("static2.example.com", 10);
}

}  // namespace
