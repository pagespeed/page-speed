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

#include "pagespeed/core/input_capabilities.h"
#include "pagespeed/core/pagespeed_input.h"
#include "pagespeed/core/resource.h"
#include "pagespeed/core/resource_filter.h"
#include "pagespeed/proto/timeline.pb.h"
#include "pagespeed/testing/instrumentation_data_builder.h"
#include "pagespeed/testing/pagespeed_test.h"

namespace {

using pagespeed::ClientCharacteristics;
using pagespeed::InputCapabilities;
using pagespeed::InstrumentationData;
using pagespeed::InstrumentationDataVector;
using pagespeed::PagespeedInput;
using pagespeed::PagespeedInputFreezeParticipant;
using pagespeed::Resource;
using pagespeed::ResourceVector;
using pagespeed_testing::AssertProtoEq;
using pagespeed_testing::FakeDomDocument;
using pagespeed_testing::FakeDomElement;

static const char* kURL1 = "http://www.foo.com/";
static const char* kURL2 = "http://www.bar.com/";


Resource* NewResource(const std::string& url, int status_code) {
  Resource* resource = new Resource;
  resource->SetRequestUrl(url);
  resource->SetResponseStatusCode(status_code);
  return resource;
}



TEST(PagespeedInputTest, SetClientCharacteristicsFailsWhenFrozen) {
  PagespeedInput input;
  ClientCharacteristics cc;
  cc.set_dns_requests_weight(100.0);
  input.Freeze();
#ifdef NDEBUG
  ASSERT_FALSE(input.SetClientCharacteristics(cc));
  ClientCharacteristics default_cc;
  AssertProtoEq(input.input_information()->client_characteristics(),
                default_cc);
#else
  ASSERT_DEATH(input.SetClientCharacteristics(cc),
               "Can't set ClientCharacteristics for frozen PagespeedInput.");
#endif
}

TEST(PagespeedInputTest, SetClientCharacteristics) {
  PagespeedInput input;
  ClientCharacteristics cc;
  cc.set_dns_requests_weight(100.0);
  ASSERT_TRUE(input.SetClientCharacteristics(cc));
  input.Freeze();
  AssertProtoEq(input.input_information()->client_characteristics(), cc);
}

TEST(PagespeedInputTest, AcquireInstrumentationData) {
  PagespeedInput input;
  InstrumentationDataVector data;
  InstrumentationData *id1 = new InstrumentationData();
  InstrumentationData *id2 = new InstrumentationData();
  data.push_back(id1);
  data.push_back(id2);
  ASSERT_TRUE(input.AcquireInstrumentationData(&data));
  ASSERT_EQ(static_cast<size_t>(0), data.size());
  input.Freeze();
  ASSERT_EQ(static_cast<size_t>(2), input.instrumentation_data()->size());
}

TEST(PagespeedInputTest, AcquireInstrumentationDataFailsWhenFrozen) {
  PagespeedInput input;
  InstrumentationDataVector data;
  input.Freeze();
#ifdef NDEBUG
  ASSERT_FALSE(input.AcquireInstrumentationData(&data));
#else
  ASSERT_DEATH(
      input.AcquireInstrumentationData(&data),
      "Can't set InstrumentationDataVector for frozen PagespeedInput.");
#endif
}

class TestFreezeParticipant : public PagespeedInputFreezeParticipant {
 public:
  virtual void OnFreeze(PagespeedInput* pagespeed_input) {
    // We must be able to access mutable resources as well as to invoke getters
    // that would only work while frozen.
    ASSERT_EQ(pagespeed_input->num_resources(), 2);
    EXPECT_EQ(pagespeed_input->GetMutableResource(0)->GetRequestUrl(), kURL1);
    EXPECT_EQ(
        pagespeed_input->GetMutableResourceWithUrlOrNull(
            kURL1)->GetRequestUrl(),
        kURL1);
    ASSERT_EQ(pagespeed_input->instrumentation_data()->size(), 1U);
  }
};

TEST(PagespeedInputTest, FreezeParticipant) {
  PagespeedInput input;
  EXPECT_TRUE(input.AddResource(NewResource(kURL1, 200)));
  EXPECT_TRUE(input.AddResource(NewResource(kURL2, 200)));

  pagespeed_testing::InstrumentationDataBuilder builder;
  InstrumentationDataVector records;
  records.push_back(builder.Layout().Get());
  input.AcquireInstrumentationData(&records);

  TestFreezeParticipant participant;
  input.Freeze(&participant);
}

class UpdateResourceTypesTest : public pagespeed_testing::PagespeedTest {
 protected:
  static const char* kRootUrl;

  virtual void DoSetUp() {
    NewPrimaryResource(kRootUrl);
    CreateHtmlHeadBodyElements();
  }

  void ResourceTypeTestHelper(const std::string& url,
                              const std::string& content_type,
                              pagespeed::ResourceType expected_resource_type) {
    Resource* resource = New200Resource(url);
    resource->AddResponseHeader("content-type", content_type);
    FakeDomElement::New(body(), "embed")->AddAttribute("src", url);
    ASSERT_EQ(expected_resource_type, resource->GetResourceType());
  }
};

const char* UpdateResourceTypesTest::kRootUrl = "http://example.com/";

TEST_F(UpdateResourceTypesTest, Script) {
  Resource* resource =
      NewScriptResource("http://example.com/foo.js", body());
  resource->SetResourceType(pagespeed::OTHER);
  resource->AddResponseHeader("content-type", "text/html");
  ASSERT_EQ(pagespeed::HTML, resource->GetResourceType());
  Freeze();
  ASSERT_EQ(pagespeed::JS, resource->GetResourceType());
}

TEST_F(UpdateResourceTypesTest, Img) {
  Resource* resource =
      NewPngResource("http://example.com/foo.png", body());
  resource->SetResourceType(pagespeed::OTHER);
  resource->RemoveResponseHeader("content-type");
  resource->AddResponseHeader("content-type", "text/html");
  ASSERT_EQ(pagespeed::HTML, resource->GetResourceType());
  Freeze();
  ASSERT_EQ(pagespeed::IMAGE, resource->GetResourceType());
}

TEST_F(UpdateResourceTypesTest, Embed) {
  const char* kFlashUrl = "http://example.com/foo.swf";
  Resource* resource = New200Resource(kFlashUrl);
  resource->AddResponseHeader("Content-Type", "application/x-shockwave-flash");
  FakeDomElement::New(body(), "embed")->AddAttribute("src", kFlashUrl);
  ASSERT_EQ(pagespeed::FLASH, resource->GetResourceType());
  Freeze();
  ASSERT_EQ(pagespeed::FLASH, resource->GetResourceType());
}

TEST_F(UpdateResourceTypesTest, Stylesheet) {
  Resource* resource =
      NewCssResource("http://example.com/foo.css", body());
  resource->SetResourceType(pagespeed::OTHER);
  resource->AddResponseHeader("content-type", "text/html");
  ASSERT_EQ(pagespeed::HTML, resource->GetResourceType());
  Freeze();
  ASSERT_EQ(pagespeed::CSS, resource->GetResourceType());
}

TEST_F(UpdateResourceTypesTest, Iframe) {
  FakeDomElement* iframe = FakeDomElement::NewIframe(body());
  Resource* resource =
      NewDocumentResource("http://example.com/iframe.html", iframe);
  resource->SetResourceType(pagespeed::OTHER);
  ASSERT_EQ(pagespeed::OTHER, resource->GetResourceType());
  Freeze();
  ASSERT_EQ(pagespeed::HTML, resource->GetResourceType());
}

TEST_F(UpdateResourceTypesTest, StylesheetInIframe) {
  FakeDomElement* iframe = FakeDomElement::NewIframe(body());
  FakeDomDocument* document =
      FakeDomDocument::New(iframe, "http://example.com/iframe.html");
  FakeDomElement* html = FakeDomElement::NewRoot(document, "html");

  // Add a resource in the iframe.
  Resource* resource =
      NewCssResource("http://example.com/foo.css", html);
  resource->SetResourceType(pagespeed::OTHER);
  resource->AddResponseHeader("content-type", "text/html");

  ASSERT_EQ(pagespeed::HTML, resource->GetResourceType());
  Freeze();
  ASSERT_EQ(pagespeed::CSS, resource->GetResourceType());
}

TEST_F(UpdateResourceTypesTest, DifferentTypesSameUrl) {
  // Create two different types of nodes in the DOM, one stylesheet
  // and one script, with the same URL. The resource type chosen by
  // the DOM visitor should be the first resource type that appears in
  // the DOM (in this case, stylesheet).

  // First add the stylesheet resource and node.
  Resource* resource =
      NewCssResource("http://example.com/foo", body());
  resource->SetResourceType(pagespeed::OTHER);
  resource->AddResponseHeader("content-type", "text/html");

  // Next add a script node with the same URL.
  FakeDomElement::NewScript(body(), "http://example.com/foo");
  ASSERT_EQ(pagespeed::HTML, resource->GetResourceType());
  Freeze();

  // Verify that the chosen type matches the first node type:
  // stylesheet.
  ASSERT_EQ(pagespeed::CSS, resource->GetResourceType());
}

TEST_F(UpdateResourceTypesTest, Audio) {
  ResourceTypeTestHelper("http://example.com/foo.mp3", "audio/mpeg",
                         pagespeed::MEDIA);
}

TEST_F(UpdateResourceTypesTest, Video) {
  ResourceTypeTestHelper("http://example.com/foo.mpg", "video/mpeg",
                         pagespeed::MEDIA);
}

TEST_F(UpdateResourceTypesTest, Pdf) {
  ResourceTypeTestHelper("http://example.com/foo.pdf", "application/pdf",
                         pagespeed::BINARY_DATA);
}

TEST_F(UpdateResourceTypesTest, BinaryOctetStream) {
  ResourceTypeTestHelper("http://example.com/foo.bin", "binary/octet-stream",
                         pagespeed::BINARY_DATA);
}

TEST_F(UpdateResourceTypesTest, Fonts) {
  ResourceTypeTestHelper("http://example.com/foo.ttf", "application/x-font-ttf",
                         pagespeed::FONT);
}

class EstimateCapabilitiesTest : public ::pagespeed_testing::PagespeedTest {};

TEST_F(EstimateCapabilitiesTest, NotFrozen) {
#ifdef NDEBUG
  ASSERT_TRUE(
      InputCapabilities(InputCapabilities::NONE).equals(
          pagespeed_input()->EstimateCapabilities()));
#else
  ASSERT_DEATH(pagespeed_input()->EstimateCapabilities(),
               "Can't estimate capabilities of non-frozen input.");
#endif
}

TEST_F(EstimateCapabilitiesTest, None) {
  Freeze();
  ASSERT_TRUE(
      InputCapabilities(InputCapabilities::NONE).equals(
          pagespeed_input()->EstimateCapabilities()));
}

TEST_F(EstimateCapabilitiesTest, Dom) {
  NewPrimaryResource("http://www.example.com/");
  Freeze();
  ASSERT_TRUE(
      InputCapabilities(InputCapabilities::DOM).equals(
          pagespeed_input()->EstimateCapabilities()));
}

TEST_F(EstimateCapabilitiesTest, TimelineData) {
  New200Resource("http://www.example.com/foo.png");
  pagespeed_testing::InstrumentationDataBuilder builder;
  AddInstrumentationData(builder.Layout().Get());
  Freeze();
  EXPECT_TRUE(InputCapabilities(InputCapabilities::TIMELINE_DATA).equals(
      pagespeed_input()->EstimateCapabilities()));
}

TEST_F(EstimateCapabilitiesTest, OnLoad) {
  SetOnloadTimeMillis(10);
  Resource* r1 = New200Resource("http://www.example.com/A");
  r1->SetRequestStartTimeMillis(11);
  Resource* r2 = New200Resource("http://www.example.com/B");
  r2->SetRequestStartTimeMillis(9);
  Freeze();
  ASSERT_TRUE(
      pagespeed_input()->EstimateCapabilities().satisfies(
          InputCapabilities(InputCapabilities::ONLOAD |
                            InputCapabilities::REQUEST_START_TIMES)));
  ASSERT_TRUE(pagespeed_input()->IsResourceLoadedAfterOnload(*r1));
  ASSERT_FALSE(pagespeed_input()->IsResourceLoadedAfterOnload(*r2));
}

TEST_F(EstimateCapabilitiesTest, RequestStartTimes) {
  New200Resource("http://www.example.com/")->SetRequestStartTimeMillis(0);
  New200Resource("http://www.example.com/b")->SetRequestStartTimeMillis(1);
  Freeze();
  ASSERT_TRUE(
      pagespeed_input()->EstimateCapabilities().satisfies(
          InputCapabilities(InputCapabilities::REQUEST_START_TIMES)));
}

TEST_F(EstimateCapabilitiesTest, RequestHeaders) {
  Resource* resource =
      New200Resource("http://www.example.com/");
  resource->AddRequestHeader("referer", "foo");
  resource->AddRequestHeader("host", "foo");
  resource->AddRequestHeader("accept-encoding", "foo");
  Freeze();
  ASSERT_TRUE(InputCapabilities(
      InputCapabilities::REQUEST_HEADERS).equals(
          pagespeed_input()->EstimateCapabilities()));
}

TEST_F(EstimateCapabilitiesTest, ResponseBody) {
  New200Resource("http://www.example.com/")->SetResponseBody("a");
  Freeze();
  ASSERT_TRUE(
      InputCapabilities(InputCapabilities::RESPONSE_BODY).equals(
          pagespeed_input()->EstimateCapabilities()));
}

}  // namespace
