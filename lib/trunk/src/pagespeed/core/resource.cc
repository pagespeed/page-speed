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

#include "pagespeed/core/resource.h"

#include <algorithm>
#include <map>
#include <string>

#include "base/logging.h"
#include "googleurl/src/gurl.h"

namespace {

const std::string& GetEmptyString() {
  static const std::string kEmptyString = "";
  return kEmptyString;
}

// normalize header name std::string by transforming it to lower case.
std::string GetNormalizedIdentifier(const std::string& name) {
  std::string result;
  result.reserve(name.size());
  std::transform(name.begin(), name.end(),
                 std::back_insert_iterator<std::string>(result),
                 tolower);
  return result;
}

}  // namespace

namespace pagespeed {

Resource::Resource() {
}

Resource::~Resource() {
}

void Resource::SetRequestUrl(const std::string& value) {
  request_url_ = value;
}

void Resource::SetRequestMethod(const std::string& value) {
  request_method_ = value;
}

void Resource::SetRequestProtocol(const std::string& value) {
  request_protocol_ = value;
}

void Resource::AddRequestHeader(const std::string& name,
                                const std::string& value) {
  const std::string key = GetNormalizedIdentifier(name);
  std::string& header = request_headers_[key];
  if (!header.empty()) {
    // In order to avoid keeping headers in a multi-map, we merge
    // duplicate headers are merged using commas.  This transformation is
    // allowed by the http 1.1 RFC.
    //
    // http://www.w3.org/Protocols/rfc2616/rfc2616-sec4.html#sec4.2
    // TODO change to preserve header structure if we need to.
    header += ",";
  }
  header += value;
}

void Resource::SetRequestBody(const std::string& value) {
  request_body_ = value;
}

void Resource::SetResponseStatusCode(int code) {
  status_code_ = code;
}

void Resource::SetResponseProtocol(const std::string& value) {
  response_protocol_ = value;
}

void Resource::AddResponseHeader(const std::string& name,
                                 const std::string& value) {
  const std::string key = GetNormalizedIdentifier(name);
  std::string& header = response_headers_[key];
  if (!header.empty()) {
    // In order to avoid keeping headers in a multi-map, we merge
    // duplicate headers are merged using commas.  This transformation is
    // allowed by the http 1.1 RFC.
    //
    // http://www.w3.org/Protocols/rfc2616/rfc2616-sec4.html#sec4.2
    // TODO change to preserve header structure if we need to.
    header += ",";
  }
  header += value;
}

void Resource::SetResponseBody(const std::string& value) {
  response_body_ = value;
}

const std::string& Resource::GetRequestUrl() const {
  return request_url_;
}

const std::string& Resource::GetRequestMethod() const {
  return request_method_;
}

const std::string& Resource::GetRequestProtocol() const {
  return request_protocol_;
}

const std::string& Resource::GetRequestHeader(
    const std::string& name) const {
  std::map<std::string, std::string>::const_iterator it =
      request_headers_.find(GetNormalizedIdentifier(name));
  if (it != request_headers_.end()) {
    return it->second;
  } else {
    return GetEmptyString();
  }
}

const std::string& Resource::GetRequestBody() const {
  return request_body_;
}

int Resource::GetResponseStatusCode() const {
  return status_code_;
}

const std::string& Resource::GetResponseProtocol() const {
  return response_protocol_;
}

const std::map<std::string, std::string>* Resource::GetResponseHeaders() const {
  return &response_headers_;
}

const std::string& Resource::GetResponseBody() const {
  return response_body_;
}

const std::map<std::string, std::string>* Resource::GetRequestHeaders() const {
  return &request_headers_;
}

const std::string& Resource::GetResponseHeader(
    const std::string& name) const {
  std::map<std::string, std::string>::const_iterator it =
      response_headers_.find(GetNormalizedIdentifier(name));
  if (it != response_headers_.end()) {
    return it->second;
  } else {
    return GetEmptyString();
  }
}

std::string Resource::GetHost() const {
  GURL url(GetRequestUrl());
  CHECK(url.is_valid());
  return url.host();
}

std::string Resource::GetProtocol() const {
  GURL url(GetRequestUrl());
  CHECK(url.is_valid());
  return url.scheme();
}

ResourceType Resource::GetResourceType() const {
  std::string type = GetResponseHeader("Content-Type");

  int separator_idx = type.find(";");
  if (separator_idx != std::string::npos) {
    type.erase(separator_idx);
  }

  if (type.find("text/") == 0) {
    if (type == "text/html") {
      return HTML;
    } else if (type == "text/css") {
      return CSS;
    } else if (type == "text/javascript") {
      return JS;
    } else {
      return TEXT;
    }
  } else if (type.find("image/") == 0) {
    return IMAGE;
  } else if (type == "application/x-javascript") {
    return JS;
  } else {
    return OTHER;
  }
}

}  // namespace pagespeed
