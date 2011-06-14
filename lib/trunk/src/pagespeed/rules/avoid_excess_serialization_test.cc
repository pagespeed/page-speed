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
#include "pagespeed/core/pagespeed_input.h"
#include "pagespeed/core/resource.h"
#include "pagespeed/core/result_provider.h"
#include "pagespeed/core/timeline.h"
#include "pagespeed/proto/pagespeed_output.pb.h"
#include "pagespeed/proto/timeline.pb.h"
#include "pagespeed/rules/avoid_excess_serialization.h"
#include "pagespeed/testing/pagespeed_test.h"

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
const std::string kTimelineTestDir = RULES_TEST_DIR_PATH;

void ReadFileToString(const std::string &path, std::string *dest) {
  std::ifstream file_stream;
  file_stream.open(path.c_str(), std::ifstream::in | std::ifstream::binary);
  dest->assign(std::istreambuf_iterator<char>(file_stream),
               std::istreambuf_iterator<char>());
  file_stream.close();
  ASSERT_GT(dest->size(), static_cast<size_t>(0));
}

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
    std::string timeline_json;
    ReadFileToString(kTimelineTestDir + test_file, &timeline_json);

    InstrumentationDataVector records;
    ASSERT_TRUE(
        pagespeed::CreateTimelineProtoFromJsonString(timeline_json, &records));

    for (InstrumentationDataVector::const_iterator iter = records.begin();
        iter != records.end(); iter++) {
      AddInstrumentationData(*iter);
    }
  }

  void CheckNoViolations() {
    ASSERT_TRUE(AppendResults());
    ASSERT_EQ(num_results(), 0);
  }

  void CheckOneViolation(const std::string &url, int trace_length) {
    ASSERT_TRUE(AppendResults());
    ASSERT_EQ(num_results(), 1);
    const Result& result = this->result(0);
    ASSERT_EQ(result.savings().critical_path_length_saved(), 1);
    ASSERT_EQ(result.resource_urls_size(), trace_length);
    ASSERT_EQ(result.resource_urls(0), url);
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
  CheckOneViolation(
      "http://pagespeed-advanced.prom.corp.google.com/load5_3.js", 4);
}

}  // namespace
