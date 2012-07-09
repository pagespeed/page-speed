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
#include <iterator>
#include <map>
#include <string>

#include "base/logging.h"
#include "base/stl_util-inl.h"
#include "googleurl/src/gurl.h"
#include "pagespeed/core/uri_util.h"

namespace {

const char* kHttp11 = "HTTP/1.1";
const char* kHttp10 = "HTTP/1.0";
const char* kHttpUnknown = "Unknown";

const std::string& GetEmptyString() {
  static const std::string kEmptyString = "";
  return kEmptyString;
}

bool IsRedirectStatusCode(int status_code) {
  return status_code == 301 ||
      status_code == 302 ||
      status_code == 303 ||
      status_code == 307;
}

bool IsBodyStatusCode(int status_code) {
  return status_code == 200 ||
      status_code == 203 ||
      status_code == 206 ||
      status_code == 304;
}

}  // namespace

namespace pagespeed {

// Using functions from namespace string_util.
using string_util::StringCaseEqual;
using string_util::StringCaseStartsWith;
using string_util::StringCaseEndsWith;

Resource::Resource()
    : response_body_modified_(false),
      status_code_(-1),
      response_protocol_(UNKNOWN_PROTOCOL),
      type_(OTHER),
      request_start_time_millis_(-1) {
}

Resource::~Resource() {
}

void Resource::SetRequestUrl(const std::string& value) {
  // We track resources by their network URL, which does not include
  // the fragment/hash. If there is a fragment/hash for the
  // resource, remove it. Note that this will also canonicalize the
  // URL.
  std::string url_no_fragment;
  if (uri_util::GetUriWithoutFragment(value,
                                      &url_no_fragment)) {
    if (url_no_fragment != value) {
      LOG(INFO) << "SetRequestUrl canonicalizing from "
                << value << " to " << url_no_fragment;
    }

#ifndef NDEBUG
    // Make sure that the new URL is canonicalized in debug builds.
    std::string canon_url = url_no_fragment;
    uri_util::CanonicalizeUrl(&canon_url);
    DCHECK(canon_url == url_no_fragment);
#endif
  }

  request_url_ = url_no_fragment;
}

void Resource::SetRequestMethod(const std::string& value) {
  request_method_ = value;
}

void Resource::AddRequestHeader(const std::string& name,
                                const std::string& value) {
  std::string& header = request_headers_[name];
  if (!header.empty()) {
    // In order to avoid keeping headers in a multi-map, we merge
    // duplicate headers are merged using commas.  This transformation is
    // allowed by the http 1.1 RFC.
    //
    // http://www.w3.org/Protocols/rfc2616/rfc2616-sec4.html#sec4.2
    // TODO(bmcquade): change to preserve header structure if we need to.
    header += ",";
  }
  header += value;
}

void Resource::SetRequestBody(const std::string& value) {
  request_body_ = value;
}

void Resource::SetResponseProtocol(const std::string& protocol) {
  if (string_util::StringCaseEqual(protocol, kHttp11)) {
    SetResponseProtocol(HTTP_11);
  } else if (string_util::StringCaseEqual(protocol, kHttp10)) {
    SetResponseProtocol(HTTP_10);
  } else {
    // Log what unknown protocol is used here.
    LOG(INFO) << "Setting unkown protocol " << protocol;
    SetResponseProtocol(UNKNOWN_PROTOCOL);
  }
}

const char* Resource::GetResponseProtocolString() const {
  switch (GetResponseProtocol()) {
    case HTTP_11:
      return kHttp11;
    case HTTP_10:
      return kHttp10;
    case UNKNOWN_PROTOCOL:
      return kHttpUnknown;
  }
  // It should not reach here.
  return kHttpUnknown;
}

void Resource::SetResponseStatusCode(int code) {
  status_code_ = code;
}

void Resource::AddResponseHeader(const std::string& name,
                                 const std::string& value) {
  std::string& header = response_headers_[name];
  if (!header.empty()) {
    // In order to avoid keeping headers in a multi-map, we merge
    // duplicate headers are merged using commas.  This transformation is
    // allowed by the http 1.1 RFC.
    //
    // http://www.w3.org/Protocols/rfc2616/rfc2616-sec4.html#sec4.2
    // TODO(bmcquade): change to preserve header structure if we need to.
    header += ",";
  }
  header += value;
}

void Resource::RemoveResponseHeader(const std::string& name) {
  response_headers_.erase(name);
}

void Resource::SetResponseBody(const std::string& value) {
  response_body_ = value;
}

void Resource::SetCookies(const std::string& cookies) {
  cookies_ = cookies;
}

void Resource::SetRequestStartTimeMillis(int start_millis) {
  if (start_millis < 0) {
    LOG(DFATAL) << "Invalid start_millis "
                << start_millis << " for " << GetRequestUrl();
    // Clamp to 0.
    start_millis = 0;
  }
  request_start_time_millis_ = start_millis;
}

void Resource::SetResourceType(ResourceType type) {
  if (GetResourceType() == REDIRECT) {
    LOG(DFATAL) << "Unable to SetResourceType for redirect.";
    return;
  }
  if (type == REDIRECT) {
    LOG(DFATAL) << "Unable to SetResourceType to redirect.";
    return;
  }
  if (!IsBodyStatusCode(status_code_)) {
    // This can happen for tracking resources that recieve 204
    // responses (e.g. images).
    LOG(INFO) << "Unable to SetResourceType for code " << status_code_;
    return;
  }
  type_ = type;
}

const std::string& Resource::GetRequestUrl() const {
  return request_url_;
}

const std::string& Resource::GetRequestMethod() const {
  return request_method_;
}

const std::string& Resource::GetRequestHeader(
    const std::string& name) const {
  HeaderMap::const_iterator it = request_headers_.find(name);
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

const Resource::HeaderMap* Resource::GetResponseHeaders() const {
  return &response_headers_;
}

const std::string& Resource::GetResponseBody() const {
  return response_body_;
}

const std::string& Resource::GetCookies() const {
  if (!cookies_.empty()) {
    // Use the user-specified cookies if available.
    return cookies_;
  }

  // NOTE: we could try to merge the Cookie and Set-Cookie headers like
  // a browser, but this is a non-trivial operation.
  const std::string& cookie_header = GetRequestHeader("Cookie");
  if (!cookie_header.empty()) {
    return cookie_header;
  }

  const std::string& set_cookie_header = GetResponseHeader("Set-Cookie");
  if (!set_cookie_header.empty()) {
    return set_cookie_header;
  }

  return GetEmptyString();
}

const Resource::HeaderMap* Resource::GetRequestHeaders() const {
  return &request_headers_;
}

const std::string& Resource::GetResponseHeader(
    const std::string& name) const {
  HeaderMap::const_iterator it = response_headers_.find(name);
  if (it != response_headers_.end()) {
    return it->second;
  } else {
    return GetEmptyString();
  }
}

std::string Resource::GetHost() const {
  GURL url(GetRequestUrl());
  if (!url.is_valid()) {
    LOG(DFATAL) << "Url parsing failed while processing "
                << GetRequestUrl();
    return "";
  } else {
    return url.host();
  }
}

std::string Resource::GetProtocol() const {
  GURL url(GetRequestUrl());
  if (!url.is_valid()) {
    LOG(DFATAL) << "Url parsing failed while processing "
                << GetRequestUrl();
    return "";
  } else {
    return url.scheme();
  }
}

ResourceType Resource::GetResourceType() const {
  // Prefer the status code to an explicitly specified type and the
  // contents of the Content-Type header.
  const int status_code = GetResponseStatusCode();
  if (IsRedirectStatusCode(status_code)) {
    return REDIRECT;
  }

  if (!IsBodyStatusCode(status_code)) {
    return OTHER;
  }

  // Next check to see if the type_ variable has been specified.
  if (type_ != OTHER) {
    return type_;
  }

  // Finally, fall back to the Content-Type header.
  std::string type = GetResponseHeader("Content-Type");

  size_t separator_idx = type.find(";");
  if (separator_idx != std::string::npos) {
    type.erase(separator_idx);
  }

  // Use case-insensitive comparisons, since MIME types are case insensitive.
  // See http://www.w3.org/Protocols/rfc1341/4_Content-Type.html
  if (StringCaseStartsWith(type, "text/")) {
    if (StringCaseEqual(type, "text/html") ||
        StringCaseEqual(type, "text/html-sandboxed")) {
      return HTML;
    } else if (StringCaseEqual(type, "text/css")) {
      return CSS;
    } else if (StringCaseStartsWith(type, "text/javascript") ||
               StringCaseStartsWith(type, "text/x-javascript") ||
               StringCaseEndsWith(type, "json") ||
               StringCaseEndsWith(type, "ecmascript") ||
               StringCaseEqual(type, "text/livescript") ||
               StringCaseEqual(type, "text/js") ||
               StringCaseEqual(type, "text/jscript") ||
               StringCaseEqual(type, "text/x-js")) {
      return JS;
    } else {
      return TEXT;
    }
  } else if (StringCaseStartsWith(type, "image/")) {
    return IMAGE;
  } else if (StringCaseStartsWith(type, "application/")) {
    if (StringCaseStartsWith(type, "application/javascript") ||
        StringCaseStartsWith(type, "application/x-javascript") ||
        StringCaseEndsWith(type, "json") ||
        StringCaseEndsWith(type, "ecmascript") ||
        StringCaseEqual(type, "application/livescript") ||
        StringCaseEqual(type, "application/jscript") ||
        StringCaseEqual(type, "application/js") ||
        StringCaseEqual(type, "application/x-js")) {
      return JS;
    } else if (StringCaseEqual(type, "application/xhtml+xml")) {
      return HTML;
    } else if (StringCaseEqual(type, "application/ce-html+xml")) {
      return HTML;
    } else if (StringCaseEqual(type, "application/xml")) {
      return TEXT;
    } else if (StringCaseEqual(type, "application/x-shockwave-flash")) {
      return FLASH;
    }
  }

  return OTHER;
}

ImageType Resource::GetImageType() const {
  if (GetResourceType() != IMAGE) {
    DCHECK(false) << "Non-image type: " << GetResourceType();
    return UNKNOWN_IMAGE_TYPE;
  }
  std::string type = GetResponseHeader("Content-Type");

  if (type.empty()) {
    // If there is no Content-Type header, then guess the type based on the
    // extension.
    const std::string path = GURL(GetRequestUrl()).path();
    if (StringCaseEndsWith(path, ".png")) {
      return PNG;
    } else if (StringCaseEndsWith(path, ".gif")) {
      return GIF;
    } else if (StringCaseEndsWith(path, ".jpg") ||
               StringCaseEndsWith(path, ".jpeg")) {
      return JPEG;
    } else if (StringCaseEndsWith(path, ".svg")) {
      return SVG;
    }
  } else {
    size_t separator_idx = type.find(";");
    if (separator_idx != std::string::npos) {
      type.erase(separator_idx);
    }

    if (StringCaseEqual(type, "image/png")) {
      return PNG;
    } else if (StringCaseEqual(type, "image/gif")) {
      return GIF;
    } else if (StringCaseEqual(type, "image/jpg") ||
               StringCaseEqual(type, "image/jpeg")) {
      return JPEG;
    } else if (StringCaseEqual(type, "image/svg+xml")) {
      return SVG;
    }
  }

  return UNKNOWN_IMAGE_TYPE;
}

bool Resource::IsRequestStartTimeLessThan(const Resource& other) const {
  if (!has_request_start_time_millis() ||
      !other.has_request_start_time_millis()) {
    LOG(DFATAL) << "Unable to compute request start times for resources: "
                << GetRequestUrl() << ", " << other.GetRequestUrl();
    return false;
  }
  return request_start_time_millis_ < other.request_start_time_millis_;
}

bool Resource::SerializeData(ResourceData* data) const {
  data->set_request_url(GetRequestUrl());
  data->set_request_method(GetRequestMethod());
  for (HeaderMap::const_iterator it = request_headers_.begin();
      it != request_headers_.end(); ++it) {
    HeaderData* header = data->add_request_headers();
    header->set_name(it->first);
    header->set_value(it->second);
  }

  if (!GetRequestBody().empty()) {
    data->set_request_body_size(GetRequestBody().size());
  }
  data->set_status_code(GetResponseStatusCode());
  data->set_response_protocol(GetResponseProtocol());
  for (HeaderMap::const_iterator it = response_headers_.begin();
      it != response_headers_.end(); ++it) {
    HeaderData* header = data->add_response_headers();
    header->set_name(it->first);
    header->set_value(it->second);
  }
  data->set_response_body_size(GetResponseBody().size());

  data->set_resource_type(GetResourceType());
  std::string mime_type = GetResponseHeader("Content-Type");
  if (!mime_type.empty()) {
    data->set_mime_type(mime_type);
  }

  return true;
}

}  // namespace pagespeed
