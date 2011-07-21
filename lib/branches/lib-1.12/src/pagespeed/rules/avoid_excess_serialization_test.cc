// Copyright 2011 Google Inc. All Rights Reserved.
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

#include <fstream>
#include <string>

#include "base/logging.h"
#include "base/scoped_ptr.h"
#include "pagespeed/core/instrumentation_data.h"
#include "pagespeed/core/pagespeed_input.h"
#include "pagespeed/core/resource.h"
#include "pagespeed/core/result_provider.h"
#include "pagespeed/proto/pagespeed_output.pb.h"
#include "pagespeed/proto/timeline.pb.h"
#include "pagespeed/rules/avoid_excess_serialization.h"
#include "pagespeed/testing/pagespeed_test.h"
#include "pagespeed/timeline/json_importer.h"

using pagespeed::InstrumentationDataVector;
using pagespeed::rules::AvoidExcessSerialization;
using pagespeed::PagespeedInput;
using pagespeed::Resource;
using pagespeed::Result;
using pagespeed::Results;
using pagespeed::ResultProvider;
using ::pagespeed_testing::PagespeedRuleTest;

namespace {
using pagespeed::InstrumentationData;

// The RULES_TEST_DIR_PATH macros are set by the gyp target that builds this
// file.
const char kTimelineTestDir[] = RULES_TEST_DIR_PATH;

class AvoidExcessSerializationTest :
  public PagespeedRuleTest<AvoidExcessSerialization> {
 protected:
  void AddTestResource(const std::string &url,
                       const int status_code,
                       const std::string &body) {
    Resource* resource = new Resource;
    resource->SetRequestUrl(url);
    resource->SetRequestMethod("GET");
    resource->SetResponseStatusCode(status_code);
    resource->SetResponseBody(body);
    AddResource(resource);
  }

  void NewScript(const std::string& url) {
    NewScriptResource(url, body(), NULL);
  }

  void SetTimelineData(const std::string& test_file) {
    std::string path = kTimelineTestDir + test_file;
    std::string timeline_json;
    ASSERT_TRUE(pagespeed_testing::ReadFileToString(path, &timeline_json));

    InstrumentationDataVector records;
    ASSERT_TRUE(
        pagespeed::timeline::CreateTimelineProtoFromJsonString(
            timeline_json, &records));

    for (InstrumentationDataVector::const_iterator iter = records.begin();
        iter != records.end(); iter++) {
      AddInstrumentationData(*iter);
    }
  }

  void CheckNoViolations() {
    ASSERT_TRUE(AppendResults());
    ASSERT_EQ(num_results(), 0);
  }

  void CheckViolations(const std::vector<std::string> urls,
                       const Result& result) {
    ASSERT_EQ(result.savings().critical_path_length_saved(), 1);
    ASSERT_EQ(result.resource_urls_size(), static_cast<int>(urls.size()));
    for (int i = 0; i < result.resource_urls_size(); ++i) {
      ASSERT_EQ(result.resource_urls(i), urls[i]);
    }
  }

  void CheckTwoViolations(const std::vector<std::string> urls1,
                          const std::vector<std::string> urls2) {
    ASSERT_TRUE(AppendResults());
    ASSERT_EQ(num_results(), 2);
    CheckViolations(urls1, this->result(0));
    CheckViolations(urls2, this->result(1));
  }
};

TEST_F(AvoidExcessSerializationTest, Load5) {
  NewPrimaryResource(
      "http://pagespeed-advanced.prom.corp.google.com/load5_no_loader.html");
  NewScript("http://pagespeed-advanced.prom.corp.google.com/load5_1.js");
  NewScript("http://pagespeed-advanced.prom.corp.google.com/load5_2.js");
  NewScript("http://pagespeed-advanced.prom.corp.google.com/load5_3.js");

  SetTimelineData("load5_no_loader.json");

  Freeze();

  // NOTE: this rule currently generates 2 suggestions, one of which
  // is a subset of the other. We should modify it to only suggest the
  // longest unique path of serialized resources. In the meantime this
  // example expects two serialization chains:
  std::vector<std::string> expected1;
  expected1.push_back(
      "http://pagespeed-advanced.prom.corp.google.com/load5_2.js");
  expected1.push_back(
      "http://pagespeed-advanced.prom.corp.google.com/load5_1.js");
  expected1.push_back(
      "http://pagespeed-advanced.prom.corp.google.com/load5_no_loader.html");
  std::vector<std::string> expected2;
  expected2.push_back(
      "http://pagespeed-advanced.prom.corp.google.com/load5_3.js");
  expected2.push_back(
      "http://pagespeed-advanced.prom.corp.google.com/load5_2.js");
  expected2.push_back(
      "http://pagespeed-advanced.prom.corp.google.com/load5_1.js");
  expected2.push_back(
      "http://pagespeed-advanced.prom.corp.google.com/load5_no_loader.html");

  CheckTwoViolations(expected1, expected2);
}

}  // namespace
