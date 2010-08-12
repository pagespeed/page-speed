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

#include "pagespeed/testing/pagespeed_test.h"

namespace pagespeed_testing {

PagespeedTest::PagespeedTest() {}
PagespeedTest::~PagespeedTest() {}

void PagespeedTest::SetUp() {
  input_.reset(new pagespeed::PagespeedInput());
  DoSetUp();
}

void PagespeedTest::TearDown() {
  DoTearDown();
  input_.reset();
}

void PagespeedTest::DoSetUp() {}
void PagespeedTest::DoTearDown() {}

void PagespeedTest::Freeze() {
  ASSERT_TRUE(input_->Freeze());
}

pagespeed::Resource* PagespeedTest::NewResource(const std::string& url,
                                                int status_code) {
  pagespeed::Resource* resource = new pagespeed::Resource();
  resource->SetRequestUrl(url);
  resource->SetRequestMethod("GET");
  resource->SetResponseStatusCode(status_code);
  input_->AddResource(resource);
  return resource;
}

pagespeed::Resource* PagespeedTest::New200Resource(const std::string& source) {
  return NewResource(source, 200);
}

pagespeed::Resource* PagespeedTest::New302Resource(
    const std::string& source, const std::string& destination) {
  pagespeed::Resource* resource = NewResource(source, 302);
  resource->AddResponseHeader("Location", destination);
  return resource;
}

bool PagespeedTest::AddResource(const pagespeed::Resource* resource) {
  return input_->AddResource(resource);
}

bool PagespeedTest::AcquireDomDocument(pagespeed::DomDocument* document) {
  return input_->AcquireDomDocument(document);
}

bool PagespeedTest::AcquireImageAttributesFactory(
    pagespeed::ImageAttributesFactory* factory) {
  return input_->AcquireImageAttributesFactory(factory);
}

bool PagespeedTest::SetPrimaryResourceUrl(const std::string& url) {
  return input_->SetPrimaryResourceUrl(url);
}

void PagespeedTest::SetAllowDuplicateResources() {
  input_->set_allow_duplicate_resources();
}

const char* PagespeedTest::kUrl1 = "http://www.example.com/a";
const char* PagespeedTest::kUrl2 = "http://www.foo.com/b";
const char* PagespeedTest::kUrl3 = "http://www.bar.com/c";
const char* PagespeedTest::kUrl4 = "http://www.hello.com/d";

}  // namespace pagespeed_testing
