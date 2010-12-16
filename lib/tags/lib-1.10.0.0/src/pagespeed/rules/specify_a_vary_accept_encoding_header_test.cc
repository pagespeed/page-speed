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
#include "pagespeed/core/pagespeed_input.h"
#include "pagespeed/core/resource.h"
#include "pagespeed/core/result_provider.h"
#include "pagespeed/proto/pagespeed_output.pb.h"
#include "pagespeed/rules/specify_a_vary_accept_encoding_header.h"
#include "pagespeed/testing/pagespeed_test.h"

using pagespeed::rules::SpecifyAVaryAcceptEncodingHeader;
using pagespeed::PagespeedInput;
using pagespeed::Resource;
using pagespeed::Result;
using pagespeed::Results;
using pagespeed::ResultProvider;
using pagespeed_testing::PagespeedRuleTest;

namespace {

class SpecifyAVaryAcceptEncodingHeaderTest
    : public PagespeedRuleTest<SpecifyAVaryAcceptEncodingHeader> {
 protected:
  void AddTestResource(const std::string& url,
                       const std::string& type,
                       const std::string& cache_control_header,
                       const std::string& vary_header) {
    Resource* resource = new Resource;
    resource->SetRequestUrl(url);
    resource->SetRequestMethod("GET");
    resource->SetResponseStatusCode(200);
    resource->SetResponseBody("Hello, world!");
    resource->AddResponseHeader("Content-Type", type);
    if (!cache_control_header.empty()) {
      resource->AddResponseHeader("Cache-Control", cache_control_header);
    }
    if (!vary_header.empty()) {
      resource->AddResponseHeader("Vary", vary_header);
    }
    AddResource(resource);
  }
};

TEST_F(SpecifyAVaryAcceptEncodingHeaderTest, NoProblems) {
  AddTestResource("http://www.example.com/index.html",
                  "text/html", "private", "");
  AddTestResource("http://www.example.com/not-static.css",
                  "text/css", "max-age=-1", "");
  AddTestResource("http://static.example.com/styles.css",
                  "text/css", "", "accept-encoding");
  AddTestResource("http://static.example.com/styles2.css",
                  "text/css", "", "Accept-Encoding,User-Agent");
  AddTestResource("http://static.example.com/styles3.css",
                  "text/css", "", "User-Agent,Accept-Encoding");
  // RFC 2616 section 14.44 specifies that the Vary header is case-insensitive,
  // so make sure that the rule can handle this:
  AddTestResource("http://static.example.com/styles4.css",
                  "text/css", "", "aCcEpT-eNcOdInG");
  CheckNoViolations();
}

TEST_F(SpecifyAVaryAcceptEncodingHeaderTest, OneViolation) {
  AddTestResource("http://www.example.com/index.html",
                  "text/html", "private", "");
  AddTestResource("http://static.example.com/styles.css",
                  "text/css", "", "");
  CheckOneUrlViolation("http://static.example.com/styles.css");
}

}  // namespace
