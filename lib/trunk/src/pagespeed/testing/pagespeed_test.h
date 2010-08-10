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
#include "pagespeed/testing/resource_builder.h"
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
  ResourceBuilder& NewResource(const std::string& url, int status_code);

  // Construct a new HTTP GET Resource with the specified URL and
  // a 200 status code, and add that resource to our PagespeedInput.
  ResourceBuilder& New200Resource(const std::string& url);

  // Construct a new HTTP GET redirect (302) Resource with the
  // specified source and destination URLs, and add that resource
  // to our PagespeedInput.
  ResourceBuilder& New302Resource(const std::string& source,
                                  const std::string& destination);

  scoped_ptr<pagespeed::PagespeedInput> input_;

 private:
  ResourceBuilder builder_;
};

}  // namespace pagespeed_testing

#endif  // PAGESPEED_TESTING_PAGESPEED_TEST_H_
