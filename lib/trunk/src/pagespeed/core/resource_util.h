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

#ifndef PAGESPEED_CORE_RESOURCE_UTIL_H_
#define PAGESPEED_CORE_RESOURCE_UTIL_H_

#include <map>

namespace pagespeed {

class Resource;

namespace resource_util {

typedef std::map<std::string, std::string> DirectiveMap;

int EstimateRequestBytes(const Resource& resource);
int EstimateResponseBytes(const Resource& resource);

// Parse directives from the given HTTP header.
// For instance, if Cache-Control contains "private, max-age=0" we
// expect the map to contain two pairs, one with key private and no
// value, and the other with key max-age and value 0. This method can
// parse headers which uses either comma (, e.g. Cache-Control) or
// semicolon (; e.g. Content-Type) as the directive separator.
bool GetHeaderDirectives(const std::string& header, DirectiveMap* out);

}  // namespace resource_util

}  // namespace pagespeed

#endif  // PAGESPEED_CORE_RESOURCE_UTIL_H_
