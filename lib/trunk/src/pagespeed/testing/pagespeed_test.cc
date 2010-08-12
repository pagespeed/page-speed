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
#include "pagespeed/core/image_attributes.h"

namespace {

void AssertNull(const void* value) {
  ASSERT_TRUE(value == NULL);
}

void AssertNotNull(const void* value) {
  ASSERT_TRUE(value != NULL);
}

class FakeImageAttributesFactory
    : public pagespeed::ImageAttributesFactory {
 public:
  virtual pagespeed::ImageAttributes* NewImageAttributes(
      const pagespeed::Resource* resource) const {
    return new pagespeed::ConcreteImageAttributes(42, 23);
  }
};

}  // namespace

namespace pagespeed_testing {

PagespeedTest::PagespeedTest() {}
PagespeedTest::~PagespeedTest() {}

void PagespeedTest::SetUp() {
  input_.reset(new pagespeed::PagespeedInput());
  document_ = NULL;
  html_ = NULL;
  head_ = NULL;
  body_ = NULL;
  DoSetUp();
}

void PagespeedTest::TearDown() {
  DoTearDown();
  document_ = NULL;
  html_ = NULL;
  head_ = NULL;
  body_ = NULL;
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

pagespeed::Resource* PagespeedTest::NewPrimaryResource(const std::string& url) {
  pagespeed::Resource* resource = New200Resource(url);
  resource->SetResourceType(pagespeed::HTML);
  AssertNull(document_);
  document_ = FakeDomDocument::NewRoot(url);
  input_->AcquireDomDocument(document_);
  input_->SetPrimaryResourceUrl(url);
  return resource;
}

pagespeed::Resource* PagespeedTest::NewDocumentResource(const std::string& url,
                                                        FakeDomElement* iframe,
                                                        FakeDomDocument** out) {
  pagespeed::Resource* resource = New200Resource(url);
  resource->SetResourceType(pagespeed::HTML);
  FakeDomDocument* document = FakeDomDocument::New(iframe, url);
  if (out != NULL) {
    *out = document;
  }
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

pagespeed::Resource* PagespeedTest::NewPngResource(const std::string& url,
                                                   FakeDomElement* parent,
                                                   FakeDomElement** out) {
  pagespeed::Resource* resource = New200Resource(url);
  resource->AddResponseHeader("Content-Type", "image/png");
  FakeDomElement* element = FakeDomElement::NewImg(parent, url);
  if (out != NULL) {
    *out = element;
  }
  return resource;
}

void PagespeedTest::CreateHtmlHeadBodyElements() {
  AssertNotNull(document());
  AssertNull(html_);
  AssertNull(head_);
  AssertNull(body_);
  html_ = FakeDomElement::NewRoot(document(), "html");
  head_ = FakeDomElement::New(html(), "head");
  body_ = FakeDomElement::New(html(), "body");
}

bool PagespeedTest::AddResource(const pagespeed::Resource* resource) {
  return input_->AddResource(resource);
}

bool PagespeedTest::AddFakeImageAttributesFactory() {
  return input_->AcquireImageAttributesFactory(
      new FakeImageAttributesFactory());
}

void PagespeedTest::SetAllowDuplicateResources() {
  input_->set_allow_duplicate_resources();
}

const char* PagespeedTest::kUrl1 = "http://www.example.com/a";
const char* PagespeedTest::kUrl2 = "http://www.foo.com/b";
const char* PagespeedTest::kUrl3 = "http://www.bar.com/c";
const char* PagespeedTest::kUrl4 = "http://www.hello.com/d";

}  // namespace pagespeed_testing
