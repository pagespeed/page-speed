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

#ifndef PAGESPEED_TESTING_PAGESPEED_TEST_H_
#define PAGESPEED_TESTING_PAGESPEED_TEST_H_

#include <map>
#include <string>

#include "base/at_exit.h"
#include "base/scoped_ptr.h"
#include "pagespeed/core/image_attributes.h"
#include "pagespeed/core/pagespeed_input.h"
#include "pagespeed/core/resource.h"
#include "pagespeed/core/result_provider.h"
#include "pagespeed/core/rule.h"
#include "pagespeed/proto/pagespeed_output.pb.h"
#include "pagespeed/testing/fake_dom.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace pagespeed {
class Resource;
}  // namespace pagespeed

namespace pagespeed_testing {

class FakeImageAttributesFactory
    : public pagespeed::ImageAttributesFactory {
 public:
  typedef std::map<const pagespeed::Resource*, std::pair<int,int> >
      ResourceSizeMap;
  explicit FakeImageAttributesFactory(const ResourceSizeMap& resource_size_map)
      : resource_size_map_(resource_size_map) {
  }
  virtual pagespeed::ImageAttributes* NewImageAttributes(
      const pagespeed::Resource* resource) const;
 private:
  ResourceSizeMap resource_size_map_;
};


// Helper method that returns the output from a TextFormatter for
// the given Rule and Results.
std::string DoFormatResults(
    pagespeed::Rule* rule, const pagespeed::Results& results);

class PagespeedTest : public ::testing::Test {
 protected:
  // Some sample URLs that tests may choose to use.
  static const char* kUrl1;
  static const char* kUrl2;
  static const char* kUrl3;
  static const char* kUrl4;

  static void SetUpTestCase();
  static void TearDownTestCase();

  PagespeedTest();
  virtual ~PagespeedTest();

  // Derived classes should not override SetUp and TearDown. Derived
  // classes should override DoSetUp and DoTearDown instead.
  virtual void SetUp();
  virtual void TearDown();

  // Hooks for derived classes to override.
  virtual void DoSetUp();
  virtual void DoTearDown();

  // Freeze the PagespeedInput structure.
  void Freeze();

  // Construct a new HTTP GET Resource with the specified URL and
  // status code, and add that resource to our PagespeedInput.
  pagespeed::Resource* NewResource(const std::string& url, int status_code);

  // Construct the primary resource, an HTTP GET HTML
  // resource with a 200 status code. An associated FakeDomDocument
  // will be created for this resource, which is stored as the DOM
  // document of the PagespeedInput. The FakeDomDocument is available
  // via the document() method. This method must only be called once
  // per test.
  pagespeed::Resource* NewPrimaryResource(const std::string& url);

  // Construct an HTTP GET HTML resource with a 200 status code. An
  // associated FakeDomDocument will be created for this resource,
  // parented under the specified iframe and returned via the
  // out_document parameter, if specified.
  pagespeed::Resource* NewDocumentResource(const std::string& url,
                                           FakeDomElement* iframe = NULL,
                                           FakeDomDocument** out = NULL);

  // Construct a new HTTP GET Resource with the specified URL and
  // a 200 status code, and add that resource to our PagespeedInput.
  pagespeed::Resource* New200Resource(const std::string& url);

  // Construct a new HTTP GET redirect (302) Resource with the
  // specified source and destination URLs, and add that resource
  // to our PagespeedInput.
  pagespeed::Resource* New302Resource(const std::string& source,
                                      const std::string& destination);

  // Construct a new HTTP GET image (PNG) resource, and add that
  // resource to our PagespeedInput. Also create an associated DOM
  // node, parented under the specified parent, and returned via the
  // out parameter, if specified.
  pagespeed::Resource* NewPngResource(const std::string& url,
                                      FakeDomElement* parent = NULL,
                                      FakeDomElement** out = NULL);

  // Construct a new HTTP GET script resource, and add that
  // resource to our PagespeedInput. Also create an associated DOM
  // node, parented under the specified parent, and returned via the
  // out parameter, if specified.
  pagespeed::Resource* NewScriptResource(const std::string& url,
                                         FakeDomElement* parent = NULL,
                                         FakeDomElement** out = NULL);

  // Construct a new HTTP GET CSS resource, and add that
  // resource to our PagespeedInput. Also create an associated DOM
  // node, parented under the specified parent, and returned via the
  // out parameter, if specified.
  pagespeed::Resource* NewCssResource(const std::string& url,
                                      FakeDomElement* parent = NULL,
                                      FakeDomElement** out = NULL);

  // Construct default html, head, and body DOM elements under the
  // document. NewPrimaryResource() must be called prior to calling
  // this method, in order to create a root document that these
  // elements can be parented under.
  void CreateHtmlHeadBodyElements();

  // Adds an ImageAttributesFactory to the PagespeedInput that can
  // returns ImageAttributes according to the ResourceSizeMap.
  bool AddFakeImageAttributesFactory(
      const FakeImageAttributesFactory::ResourceSizeMap& map);

  // Enable duplicate resources in the PagespeedInput. Most tests
  // should not need to call this, since the default PagespeedInput
  // behavior is to not allow duplicates.
  void SetAllowDuplicateResources();

  const pagespeed::PagespeedInput* input() { return input_.get(); }
  pagespeed::Resource* primary_resource() const { return primary_resource_; }
  FakeDomDocument* document() { return document_; }
  FakeDomElement* html() { return html_; }
  FakeDomElement* head() { return head_; }
  FakeDomElement* body() { return body_; }

  // Add a resource. Do not call this method for resources constructed
  // using New*Resource, as those resources have already been added to
  // the PagespeedInput. Use this method only for backward
  // compatibility with tests that don't use the New*Resource()
  // methods to construct resouces.
  bool AddResource(pagespeed::Resource* resource);

 private:
  base::AtExitManager at_exit_manager_;
  scoped_ptr<pagespeed::PagespeedInput> input_;
  pagespeed::Resource* primary_resource_;
  FakeDomDocument* document_;
  FakeDomElement* html_;
  FakeDomElement* head_;
  FakeDomElement* body_;
};

// A base testing class for use when writing rule tests.
template <class RULE> class PagespeedRuleTest : public PagespeedTest {
 protected:
  PagespeedRuleTest()
      : rule_(new RULE()), provider_(*rule_.get(), &results_) {}

  const pagespeed::Results& results() const { return results_; }
  const int num_results() const { return results_.results_size(); }
  const pagespeed::Result& result(int i) const { return results_.results(i); }

  bool AppendResults() { return rule_->AppendResults(*input(), &provider_); }
  std::string FormatResults() { return DoFormatResults(rule_.get(), results_); }

  int ComputeScore() {
    pagespeed::ResultVector r;
    for (int idx = 0, end = num_results(); idx < end; ++idx) {
      r.push_back(&results().results(idx));
    }
    return rule_->ComputeScore(*input()->input_information(), r);
  }

 private:
  scoped_ptr<pagespeed::Rule> rule_;
  pagespeed::Results results_;
  pagespeed::ResultProvider provider_;
};

}  // namespace pagespeed_testing

#endif  // PAGESPEED_TESTING_PAGESPEED_TEST_H_
