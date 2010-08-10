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

#include "base/logging.h"
#include "pagespeed/testing/resource_builder.h"

namespace pagespeed_testing {

ResourceBuilder::ResourceBuilder() {}

ResourceBuilder::~ResourceBuilder() {
  // Make sure that there wasn't a partially configured resource,
  // since calls to Peek() might depend on its living after this
  // builder. The user of this class must call Get() for each call to
  // Reset().
  CHECK(resource_ == NULL);
}

ResourceBuilder& ResourceBuilder::SetRequestUrl(const std::string& value) {
  DCHECK(resource_ != NULL);
  if (resource_ != NULL) {
    resource_->SetRequestUrl(value);
  }
  return *this;
}

ResourceBuilder& ResourceBuilder::SetRequestMethod(const std::string& value) {
  DCHECK(resource_ != NULL);
  if (resource_ != NULL) {
    resource_->SetRequestMethod(value);
  }
  return *this;
}

ResourceBuilder& ResourceBuilder::SetRequestProtocol(const std::string& value) {
  DCHECK(resource_ != NULL);
  if (resource_ != NULL) {
    resource_->SetRequestProtocol(value);
  }
  return *this;
}

ResourceBuilder& ResourceBuilder::AddRequestHeader(const std::string& name,
                                                   const std::string& value) {
  DCHECK(resource_ != NULL);
  if (resource_ != NULL) {
    resource_->AddRequestHeader(name, value);
  }
  return *this;
}

ResourceBuilder& ResourceBuilder::SetRequestBody(const std::string& value) {
  DCHECK(resource_ != NULL);
  if (resource_ != NULL) {
    resource_->SetRequestBody(value);
  }
  return *this;
}

ResourceBuilder& ResourceBuilder::SetResponseStatusCode(int code) {
  DCHECK(resource_ != NULL);
  if (resource_ != NULL) {
    resource_->SetResponseStatusCode(code);
  }
  return *this;
}

ResourceBuilder& ResourceBuilder::SetResponseProtocol(
    const std::string& value) {
  DCHECK(resource_ != NULL);
  if (resource_ != NULL) {
    resource_->SetResponseProtocol(value);
  }
  return *this;
}

ResourceBuilder& ResourceBuilder::AddResponseHeader(const std::string& name,
                                                    const std::string& value) {
  DCHECK(resource_ != NULL);
  if (resource_ != NULL) {
    resource_->AddResponseHeader(name, value);
  }
  return *this;
}

ResourceBuilder& ResourceBuilder::SetResponseBody(const std::string& value) {
  DCHECK(resource_ != NULL);
  if (resource_ != NULL) {
    resource_->SetResponseBody(value);
  }
  return *this;
}

ResourceBuilder& ResourceBuilder::SetCookies(const std::string& cookies) {
  DCHECK(resource_ != NULL);
  if (resource_ != NULL) {
    resource_->SetCookies(cookies);
  }
  return *this;
}

ResourceBuilder& ResourceBuilder::SetLazyLoaded() {
  DCHECK(resource_ != NULL);
  if (resource_ != NULL) {
    resource_->SetLazyLoaded();
  }
  return *this;
}

void ResourceBuilder::Reset() {
  // Make sure that there wasn't a partially configured resource,
  // since calls to Peek() might depend on its living after this call
  // to Reset().
  CHECK(resource_ == NULL);
  resource_.reset(new pagespeed::Resource());
}

pagespeed::Resource* ResourceBuilder::Get() {
  CHECK(resource_ != NULL);
  return resource_.release();
}

pagespeed::Resource* ResourceBuilder::Peek() {
  CHECK(resource_ != NULL);
  return resource_.get();
}

}  // namespace pagespeed_testing
