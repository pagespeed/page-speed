// Copyright 2009 Google Inc. All Rights Reserved.
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

#ifndef PAGESPEED_CORE_RESOURCE_H_
#define PAGESPEED_CORE_RESOURCE_H_

#include <map>
#include <set>
#include <string>

#include "base/basictypes.h"

namespace pagespeed {

class Resource;

// sorts resources by their URLs.
struct ResourceUrlLessThan {
  bool operator() (const Resource* lhs, const Resource* rhs) const;
};

typedef std::set<const Resource*, ResourceUrlLessThan> ResourceSet;

enum ResourceType {
  HTML,
  TEXT,
  CSS,
  IMAGE,
  JS,
  REDIRECT,
  FLASH,
  OTHER
};

enum ImageType {
  JPEG,
  PNG,
  GIF,
  UNKNOWN_IMAGE_TYPE
};

/**
 * Represents an individual input resource.
 */
class Resource {
 public:
  Resource();
  virtual ~Resource();

  // Setter methods
  void SetRequestUrl(const std::string& value);
  void SetRequestMethod(const std::string& value);
  void SetRequestProtocol(const std::string& value);
  void AddRequestHeader(const std::string& name, const std::string& value);
  void SetRequestBody(const std::string& value);
  void SetResponseStatusCode(int code);
  void SetResponseProtocol(const std::string& value);
  void AddResponseHeader(const std::string& name, const std::string& value);
  void SetResponseBody(const std::string& value);

  // In some cases, the Cookie header can differ from the cookie(s)
  // that would be associated with a resource. For instance, if a resource
  // is fetched before a Set-Cookie is applied, the cookies in that
  // Set-Cookie will not be included in the request for the resource. Some
  // rules want to know about the cookies that would be applied to a
  // resource. You can use the SetCookies method to specify the set of
  // cookies that are associated with a given resource. This is optional;
  // if unspecified, GetCookies will return the contents of the Cookie
  // header.
  void SetCookies(const std::string& cookies);

  // The resource is lazy-loaded if the request time is after
  // the window's onLoad time. Many of the page-speed rules
  // do not apply to lazy-loaded resources.
  void SetLazyLoaded();

  // Accessor methods
  const std::string& GetRequestUrl() const;
  const std::string& GetRequestMethod() const;
  const std::string& GetRequestProtocol() const;
  const std::string& GetRequestHeader(const std::string& name) const;
  const std::string& GetRequestBody() const;
  int GetResponseStatusCode() const;
  const std::string& GetResponseProtocol() const;
  const std::string& GetResponseHeader(const std::string& name) const;
  const std::string& GetResponseBody() const;

  // Get the cookies specified via SetCookies. If SetCookies was
  // unspecified, this will fall back to the Cookie request header. If that
  // header is empty, this method falls back to the Set-Cookie response
  // header.
  const std::string& GetCookies() const;

  bool IsLazyLoaded() const;

  // For serialization purposes only.
  // Use GetRequestHeader/GetResponseHeader methods above for key lookup.
  const std::map<std::string, std::string>* GetRequestHeaders() const;
  const std::map<std::string, std::string>* GetResponseHeaders() const;

  // Helper methods

  // extract the host std::string from the request url
  std::string GetHost() const;

  // extract the protocol std::string from the request url
  std::string GetProtocol() const;

  // Extract resource type from the Content-Type header.
  ResourceType GetResourceType() const;
  ImageType GetImageType() const;

 private:
  std::string request_url_;
  std::string request_method_;
  std::string request_protocol_;
  std::map<std::string, std::string> request_headers_;
  std::string request_body_;
  int status_code_;
  std::string response_protocol_;
  std::map<std::string, std::string> response_headers_;
  std::string response_body_;
  std::string cookies_;
  bool lazy_loaded_;

  DISALLOW_COPY_AND_ASSIGN(Resource);
};

}  // namespace pagespeed

#endif  // PAGESPEED_CORE_RESOURCE_H_
