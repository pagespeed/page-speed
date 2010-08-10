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

#ifndef PAGESPEED_TESTING_RESOURCE_BUILDER_H_
#define PAGESPEED_TESTING_RESOURCE_BUILDER_H_

#include <string>

#include "base/scoped_ptr.h"
#include "pagespeed/core/resource.h"

namespace pagespeed_testing {

class ResourceBuilder {
 public:
  ResourceBuilder();
  ~ResourceBuilder();

  // Builder methods: used to populate the Resource instance. There
  // should be one Builder method for each setter on the Resource class.
  ResourceBuilder& SetRequestUrl(const std::string& value);
  ResourceBuilder& SetRequestMethod(const std::string& value);
  ResourceBuilder& SetRequestProtocol(const std::string& value);
  ResourceBuilder& AddRequestHeader(const std::string& name,
                                    const std::string& value);
  ResourceBuilder& SetRequestBody(const std::string& value);
  ResourceBuilder& SetResponseStatusCode(int code);
  ResourceBuilder& SetResponseProtocol(const std::string& value);
  ResourceBuilder& AddResponseHeader(const std::string& name,
                                     const std::string& value);
  ResourceBuilder& SetResponseBody(const std::string& value);
  ResourceBuilder& SetCookies(const std::string& cookies);
  ResourceBuilder& SetLazyLoaded();

  // Reset the state of the builder. Users of this class much call
  // Get() after each call to Reset(), to make sure that ownership of the
  // resource is transferred from the ResourceBuilder class.
  void Reset();

  // Returns the built Resource instance. Ownership of the Resource is
  // transferred to the caller.
  pagespeed::Resource* Get();

 private:
  // PagespeedTest is allowed to reach into the private members of the
  // ResourceBuilder. These two classes work together to make it easier to
  // write Resource-based tests.
  friend class PagespeedTest;

  // Get a pointer to the resource currently being
  // built. Ownership of the Resource is not transferred to the
  // caller.
  pagespeed::Resource* Peek();

  scoped_ptr<pagespeed::Resource> resource_;
};

}  // namespace pagespeed_testing

#endif  // PAGESPEED_TESTING_RESOURCE_BUILDER_H_
