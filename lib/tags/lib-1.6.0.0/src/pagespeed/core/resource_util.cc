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

#include "pagespeed/core/resource_util.h"

#include "pagespeed/core/resource.h"

namespace {

// each message header has a 3 byte overhead; colon between the key
// value pair and the end-of-line CRLF.
const int kHeaderOverhead = 3;

int EstimateHeadersBytes(const std::map<std::string, std::string>& headers) {
  int total_size = 0;

  // TODO improve the header size calculation below.
  for (std::map<std::string, std::string>::const_iterator
           iter = headers.begin(), end = headers.end();
       iter != end;
       ++iter) {
    total_size += kHeaderOverhead + iter->first.size() + iter->second.size();
  }

  return total_size;
}

}  // namespace

namespace pagespeed {

namespace resource_util {

int EstimateRequestBytes(const Resource& resource) {
  int request_bytes = 0;

  // Request line
  request_bytes += resource.GetRequestMethod().size() + 1 /* space */ +
      resource.GetRequestUrl().size()  + 1 /* space */ +
      resource.GetRequestProtocol().size() + 2 /* \r\n */;

  request_bytes += EstimateHeadersBytes(*resource.GetRequestHeaders());
  request_bytes += resource.GetRequestBody().size();

  return request_bytes;
}

int EstimateResponseBytes(const Resource& resource) {
  int response_bytes = 0;
  // TODO get compressed size or replace with section with actual
  // download size.
  // TODO improve the header size calculation below.
  response_bytes += resource.GetResponseBody().size();
  response_bytes += resource.GetResponseProtocol().size();
  response_bytes += EstimateHeadersBytes(*resource.GetResponseHeaders());
  return response_bytes;
}

}  // namespace resource_util

}  // namespace pagespeed
