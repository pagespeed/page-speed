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

#include <string>

#include "base/scoped_ptr.h"
#include "pagespeed/core/pagespeed_input.h"
#include "pagespeed/testing/fake_dom.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace pagespeed {
class Resource;
}  // namespace pagespeed

namespace pagespeed_testing {

class PagespeedTest : public ::testing::Test {
 protected:
  // Some sample URLs that tests may choose to use.
  static const char* kUrl1;
  static const char* kUrl2;
  static const char* kUrl3;
  static const char* kUrl4;

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
                                           FakeDomElement* iframe,
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
                                      FakeDomElement* parent,
                                      FakeDomElement** out = NULL);

  // Construct a new HTTP GET script resource, and add that
  // resource to our PagespeedInput. Also create an associated DOM
  // node, parented under the specified parent, and returned via the
  // out parameter, if specified.
  pagespeed::Resource* NewScriptResource(const std::string& url,
                                         FakeDomElement* parent,
                                         FakeDomElement** out = NULL);

  // Construct a new HTTP GET CSS resource, and add that
  // resource to our PagespeedInput. Also create an associated DOM
  // node, parented under the specified parent, and returned via the
  // out parameter, if specified.
  pagespeed::Resource* NewCssResource(const std::string& url,
                                      FakeDomElement* parent,
                                      FakeDomElement** out = NULL);

  // Construct default html, head, and body DOM elements under the
  // document. NewPrimaryResource() must be called prior to calling
  // this method, in order to create a root document that these
  // elements can be parented under.
  void CreateHtmlHeadBodyElements();

  // Adds an ImageAttributesFactory to the PagespeedInput that always
  // returns ImageAttributes with width 42 and height 23.
  bool AddFakeImageAttributesFactory();

  // Enable duplicate resources in the PagespeedInput. Most tests
  // should not need to call this, since the default PagespeedInput
  // behavior is to not allow duplicates.
  void SetAllowDuplicateResources();

  const pagespeed::PagespeedInput* input() { return input_.get(); }
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
  scoped_ptr<pagespeed::PagespeedInput> input_;
  FakeDomDocument* document_;
  FakeDomElement* html_;
  FakeDomElement* head_;
  FakeDomElement* body_;
};

}  // namespace pagespeed_testing

#endif  // PAGESPEED_TESTING_PAGESPEED_TEST_H_
