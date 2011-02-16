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

#include <sstream>
#include <string>

#include "pagespeed/core/pagespeed_init.h"
#include "pagespeed/formatters/proto_formatter.h"
#include "pagespeed/l10n/localizer.h"
#include "pagespeed/proto/pagespeed_proto_formatter.pb.h"
#include "pagespeed/testing/formatted_results_test_converter.h"

namespace {

void AssertNull(const void* value) {
  ASSERT_TRUE(value == NULL);
}

void AssertNotNull(const void* value) {
  ASSERT_TRUE(value != NULL);
}

}  // namespace

namespace pagespeed_testing {

pagespeed::ImageAttributes* FakeImageAttributesFactory::NewImageAttributes(
    const pagespeed::Resource* resource) const {
  ResourceSizeMap::const_iterator c_it = resource_size_map_.find(resource);
  if (c_it == resource_size_map_.end()) {
    return NULL;
  } else {
    return new pagespeed::ConcreteImageAttributes(c_it->second.first,
                                                  c_it->second.second);
  }
}


PagespeedTest::PagespeedTest() {}
PagespeedTest::~PagespeedTest() {}

void PagespeedTest::SetUp() {
  pagespeed_input_.reset(new pagespeed::PagespeedInput());
  primary_resource_ = NULL;
  document_ = NULL;
  html_ = NULL;
  head_ = NULL;
  body_ = NULL;
  DoSetUp();
}

void PagespeedTest::TearDown() {
  DoTearDown();
  primary_resource_ = NULL;
  document_ = NULL;
  html_ = NULL;
  head_ = NULL;
  body_ = NULL;
  pagespeed_input_.reset();
}

void PagespeedTest::DoSetUp() {}
void PagespeedTest::DoTearDown() {}

void PagespeedTest::Freeze() {
  ASSERT_TRUE(pagespeed_input_->Freeze());
}

pagespeed::Resource* PagespeedTest::NewResource(const std::string& url,
                                                int status_code) {
  pagespeed::Resource* resource = new pagespeed::Resource();
  resource->SetRequestUrl(url);
  resource->SetRequestMethod("GET");
  resource->SetResponseStatusCode(status_code);
  if (!pagespeed_input_->AddResource(resource))
    return NULL; // resource deleted in AddResource
  return resource;
}

pagespeed::Resource* PagespeedTest::NewPrimaryResource(const std::string& url) {
  pagespeed::Resource* resource = New200Resource(url);
  resource->SetResourceType(pagespeed::HTML);
  AssertNull(document_);
  document_ = FakeDomDocument::NewRoot(url);
  pagespeed_input_->AcquireDomDocument(document_);
  pagespeed_input_->SetPrimaryResourceUrl(url);
  primary_resource_ = resource;
  return resource;
}

pagespeed::Resource* PagespeedTest::NewDocumentResource(const std::string& url,
                                                        FakeDomElement* iframe,
                                                        FakeDomDocument** out) {
  pagespeed::Resource* resource = New200Resource(url);
  resource->SetResourceType(pagespeed::HTML);
  if (iframe != NULL) {
    FakeDomDocument* document = FakeDomDocument::New(iframe, url);
    if (out != NULL) {
      *out = document;
    }
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
  if (parent != NULL) {
    FakeDomElement* element = FakeDomElement::NewImg(parent, url);
    if (out != NULL) {
      *out = element;
    }
  }
  return resource;
}

pagespeed::Resource* PagespeedTest::NewScriptResource(const std::string& url,
                                                      FakeDomElement* parent,
                                                      FakeDomElement** out) {
  pagespeed::Resource* resource = New200Resource(url);
  resource->SetResourceType(pagespeed::JS);
  if (parent != NULL) {
    FakeDomElement* element = FakeDomElement::NewScript(parent, url);
    if (out != NULL) {
      *out = element;
    }
  }
  return resource;
}

pagespeed::Resource* PagespeedTest::NewCssResource(const std::string& url,
                                                   FakeDomElement* parent,
                                                   FakeDomElement** out) {
  pagespeed::Resource* resource = New200Resource(url);
  resource->SetResourceType(pagespeed::CSS);
  if (parent != NULL) {
    FakeDomElement* element = FakeDomElement::NewLinkStylesheet(parent, url);
    if (out != NULL) {
      *out = element;
    }
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

bool PagespeedTest::AddResource(pagespeed::Resource* resource) {
  return pagespeed_input_->AddResource(resource);
}

bool PagespeedTest::AddFakeImageAttributesFactory(
    const FakeImageAttributesFactory::ResourceSizeMap& map) {
  return pagespeed_input_->AcquireImageAttributesFactory(
      new FakeImageAttributesFactory(map));
}

std::string DoFormatResults(
    pagespeed::Rule* rule, const pagespeed::RuleResults& rule_results) {
  pagespeed::ResultVector result_vector;
  for (int i = 0; i < rule_results.results_size(); ++i) {
    result_vector.push_back(&rule_results.results(i));
  }

  pagespeed::l10n::BasicLocalizer localizer;
  pagespeed::FormattedResults results;
  results.set_locale("en_US");
  pagespeed::formatters::ProtoFormatter rule_formatter(&localizer, &results);
  pagespeed::Formatter* formatter =
      rule_formatter.AddHeader(*rule, rule_results.rule_score());
  rule->FormatResults(result_vector, formatter);
  rule_formatter.Done();
  std::string out;
  FormattedResultsTestConverter::Convert(results, &out);
  return out;
}

const char* PagespeedTest::kUrl1 = "http://www.example.com/a";
const char* PagespeedTest::kUrl2 = "http://www.foo.com/b";
const char* PagespeedTest::kUrl3 = "http://www.bar.com/c";
const char* PagespeedTest::kUrl4 = "http://www.hello.com/d";

}  // namespace pagespeed_testing
