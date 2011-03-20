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

#include "pagespeed/core/javascript_call_info.h"
#include "pagespeed/proto/pagespeed_output.pb.h"
#include "pagespeed/rules/avoid_document_write.h"
#include "pagespeed/testing/pagespeed_test.h"

namespace {

using pagespeed::AvoidDocumentWriteDetails;
using pagespeed::rules::AvoidDocumentWrite;
using pagespeed_testing::FakeDomDocument;
using pagespeed_testing::FakeDomElement;

class AvoidDocumentWriteTest
    : public ::pagespeed_testing::PagespeedRuleTest<AvoidDocumentWrite> {
 protected:
  static const char* kRootUrl;
  static const size_t kImgSizeBytes = 50;

  virtual void DoSetUp() {
    NewPrimaryResource(kRootUrl);
    CreateHtmlHeadBodyElements();
  }

  pagespeed::Resource* NewPngResourceWithBody(
      const std::string& url, FakeDomElement* parent) {
    pagespeed::Resource* resource = NewPngResource(url, parent);
    std::string body(kImgSizeBytes, 'x');
    resource->SetResponseBody(body);
    return resource;
  }

  void AssertTrue(bool statement) { ASSERT_TRUE(statement); }

  const pagespeed::AvoidDocumentWriteDetails& details(int result_idx) {
    AssertTrue(result(result_idx).has_details());
    const pagespeed::ResultDetails& details = result(result_idx).details();
    AssertTrue(details.HasExtension(
        AvoidDocumentWriteDetails::message_set_extension));
    return details.GetExtension(
        AvoidDocumentWriteDetails::message_set_extension);
  }

  void AddDocumentWriteCall(pagespeed::Resource* resource,
                            const pagespeed::Resource* parent_resource,
                            const std::string& src) {
    std::vector<std::string> args;
    args.push_back(src);
    const pagespeed::JavaScriptCallInfo* info =
        new pagespeed::JavaScriptCallInfo("document.write",
                                          parent_resource->GetRequestUrl(),
                                          args,
                                          1);
    resource->AddJavaScriptCall(info);
  }
};

const char* AvoidDocumentWriteTest::kRootUrl = "http://example.com/";

TEST_F(AvoidDocumentWriteTest, NoJSCalls) {
  Freeze();
  AppendResults();
  ASSERT_EQ(0, num_results());
}

TEST_F(AvoidDocumentWriteTest, DocumentWriteNoExternalResources) {
  AddDocumentWriteCall(
      primary_resource(), primary_resource(), "<p>Hello!</p>");
  Freeze();
  AppendResults();
  ASSERT_EQ(0, num_results());
}

TEST_F(AvoidDocumentWriteTest, NoBlockedResourcesFromDocument) {
  AddDocumentWriteCall(
      primary_resource(), primary_resource(), "<script src='foo.js'></script>");
  NewPngResourceWithBody("http://example.com/foo.png", body());
  NewScriptResource("http://example.com/foo.js", body());
  Freeze();
  AppendResults();
  ASSERT_EQ(0, num_results());
}

TEST_F(AvoidDocumentWriteTest, NoBlockedResourcesFromScript) {
  pagespeed::Resource* script_resource =
      NewScriptResource("http://example.com/a.js", body());
  AddDocumentWriteCall(
      script_resource, primary_resource(), "<script src='foo.js'></script>");
  NewPngResourceWithBody("http://example.com/foo.png", body());
  NewScriptResource("http://example.com/foo.js", body());
  Freeze();
  AppendResults();
  ASSERT_EQ(0, num_results());
}

TEST_F(AvoidDocumentWriteTest, BlockedPngFromDocument) {
  AddDocumentWriteCall(
      primary_resource(), primary_resource(), "<script src='foo.js'></script>");
  NewScriptResource("http://example.com/foo.js", body());
  NewPngResourceWithBody("http://example.com/foo.png", body());
  Freeze();
  AppendResults();
  ASSERT_EQ(1, num_results());
  ASSERT_EQ(1, result(0).resource_urls_size());
  ASSERT_EQ(kRootUrl, result(0).resource_urls(0));
  ASSERT_EQ(1, details(0).urls_size());
  ASSERT_EQ("http://example.com/foo.js", details(0).urls(0));
  ASSERT_EQ(1, details(0).line_number());
}

TEST_F(AvoidDocumentWriteTest, BlockedPngFromScript) {
  pagespeed::Resource* script_resource =
      NewScriptResource("http://example.com/a.js", body());
  AddDocumentWriteCall(
      script_resource, primary_resource(), "<script src='foo.js'></script>");
  NewScriptResource("http://example.com/foo.js", body());
  NewPngResourceWithBody("http://example.com/foo.png", body());
  Freeze();
  AppendResults();
  ASSERT_EQ(1, num_results());
  ASSERT_EQ(1, result(0).resource_urls_size());
  ASSERT_EQ("http://example.com/a.js", result(0).resource_urls(0));
  ASSERT_EQ(1, details(0).urls_size());
  ASSERT_EQ("http://example.com/foo.js", details(0).urls(0));
  ASSERT_EQ(1, details(0).line_number());
}

TEST_F(AvoidDocumentWriteTest, MultipleScriptSrc) {
  AddDocumentWriteCall(
      primary_resource(),
      primary_resource(),
      "<script src='foo.js'></script><script src='bar.js'></script>");
  NewScriptResource("http://example.com/foo.js", body());
  NewScriptResource("http://example.com/bar.js", body());
  NewPngResourceWithBody("http://example.com/foo.png", body());
  Freeze();
  AppendResults();
  ASSERT_EQ(1, num_results());
  ASSERT_EQ(1, result(0).resource_urls_size());
  ASSERT_EQ(kRootUrl, result(0).resource_urls(0));
  ASSERT_EQ(2, details(0).urls_size());
  ASSERT_EQ("http://example.com/bar.js", details(0).urls(0));
  ASSERT_EQ("http://example.com/foo.js", details(0).urls(1));
  ASSERT_EQ(1, details(0).line_number());
}

TEST_F(AvoidDocumentWriteTest, MultipleDocumentWriteCalls) {
  AddDocumentWriteCall(
      primary_resource(), primary_resource(), "<script src='a.js'></script>");
  AddDocumentWriteCall(
      primary_resource(), primary_resource(), "<script src='b.js'></script>");
  NewScriptResource("http://example.com/a.js", body());
  NewScriptResource("http://example.com/b.js", body());
  NewPngResourceWithBody("http://example.com/foo.png", body());
  Freeze();
  AppendResults();
  ASSERT_EQ(2, num_results());
  ASSERT_EQ(1, result(0).resource_urls_size());
  ASSERT_EQ(kRootUrl, result(0).resource_urls(0));
  ASSERT_EQ(1, details(0).urls_size());
  ASSERT_EQ("http://example.com/a.js", details(0).urls(0));

  ASSERT_EQ(1, result(1).resource_urls_size());
  ASSERT_EQ(kRootUrl, result(1).resource_urls(0));
  ASSERT_EQ(1, details(1).urls_size());
  ASSERT_EQ("http://example.com/b.js", details(1).urls(0));
}

TEST_F(AvoidDocumentWriteTest, NoBlockedResourcesInIframe) {
  FakeDomElement* iframe = FakeDomElement::NewIframe(body());
  FakeDomDocument* iframe_doc;
  pagespeed::Resource* iframe_resource =
      NewDocumentResource("http://example.com/iframe/", iframe, &iframe_doc);
  FakeDomElement* iframe_root = FakeDomElement::NewRoot(iframe_doc, "html");
  AddDocumentWriteCall(
      iframe_resource,
      iframe_resource,
      "<script src='foo.js'></script>");
  NewScriptResource("http://example.com/iframe/foo.js", iframe_root);
  NewScriptResource("http://example.com/iframe/foo2.js", iframe_root);

  // Also insert a PNG resource in the main document, after the
  // iframe. This should not trigger a violation since parsing the
  // iframe does not block parsing of the main document.
  NewPngResourceWithBody("http://example.com/foo.png", body());
  Freeze();
  AppendResults();
  ASSERT_EQ(0, num_results());
}

TEST_F(AvoidDocumentWriteTest, BlockedPngInIframe) {
  FakeDomElement* iframe = FakeDomElement::NewIframe(body());
  FakeDomDocument* iframe_doc;
  pagespeed::Resource* iframe_resource =
      NewDocumentResource("http://example.com/iframe/", iframe, &iframe_doc);
  FakeDomElement* iframe_root = FakeDomElement::NewRoot(iframe_doc, "html");
  AddDocumentWriteCall(
      iframe_resource,
      iframe_resource,
      "<script src='foo.js'></script>");
  NewScriptResource("http://example.com/iframe/foo.js", iframe_root);
  NewScriptResource("http://example.com/iframe/foo2.js", iframe_root);
  NewPngResourceWithBody("http://example.com/foo.png", iframe_root);
  Freeze();
  AppendResults();
  ASSERT_EQ(1, num_results());
  ASSERT_EQ(1, result(0).resource_urls_size());
  ASSERT_EQ("http://example.com/iframe/", result(0).resource_urls(0));
  ASSERT_EQ(1, details(0).urls_size());
  ASSERT_EQ("http://example.com/iframe/foo.js", details(0).urls(0));
}

TEST_F(AvoidDocumentWriteTest, DocumentWriteOneMissingResource) {
  AddDocumentWriteCall(
      primary_resource(),
      primary_resource(),
      "<script src='foo.js'></script><script src='bar.js'></script>");
  // Add foo.js, but not bar.js.
  NewScriptResource("http://example.com/foo.js", body());
  NewPngResourceWithBody("http://example.com/foo.png", body());
  Freeze();
  AppendResults();
  ASSERT_EQ(1, num_results());
  ASSERT_EQ(1, result(0).resource_urls_size());
  ASSERT_EQ(kRootUrl, result(0).resource_urls(0));
  ASSERT_EQ(2, details(0).urls_size());
  ASSERT_EQ("http://example.com/bar.js", details(0).urls(0));
  ASSERT_EQ("http://example.com/foo.js", details(0).urls(1));
  ASSERT_EQ(1, details(0).line_number());
}

TEST_F(AvoidDocumentWriteTest, DocumentWriteMissingResources) {
  AddDocumentWriteCall(
      primary_resource(),
      primary_resource(),
      "<script src='foo.js'></script><script src='bar.js'></script>");
  // Add neither foo.js nor bar.js.
  NewPngResourceWithBody("http://example.com/foo.png", body());
  Freeze();
  AppendResults();
  ASSERT_EQ(0, num_results());
}

TEST_F(AvoidDocumentWriteTest, DocumentWriteRedirected) {
  AddDocumentWriteCall(primary_resource(),
                       primary_resource(),
                       "<script src='foo.js'></script>");
  FakeDomElement::NewScript(body(), "foo.js");
  New302Resource("http://example.com/foo.js",
                 "http://example.com/redirected.js");
  NewScriptResource("http://example.com/redirected.js");
  NewPngResourceWithBody("http://example.com/foo.png", body());
  Freeze();
  AppendResults();
  ASSERT_EQ(1, num_results());
  ASSERT_EQ(1, result(0).resource_urls_size());
  ASSERT_EQ(kRootUrl, result(0).resource_urls(0));
  ASSERT_EQ(1, details(0).urls_size());
  ASSERT_EQ("http://example.com/foo.js", details(0).urls(0));
  ASSERT_EQ(1, details(0).line_number());
}

TEST_F(AvoidDocumentWriteTest, PostOnloadPng) {
  SetOnloadTimeMillis(10);
  pagespeed::Resource* script_resource =
      NewScriptResource("http://example.com/a.js", body());
  AddDocumentWriteCall(
      script_resource, primary_resource(), "<script src='foo.js'></script>");
  NewScriptResource("http://example.com/foo.js", body());
  NewPngResourceWithBody(
      "http://example.com/foo.png", body())->SetRequestStartTimeMillis(11);
  Freeze();
  AppendResults();
  ASSERT_EQ(0, num_results());
}

}  // namespace
