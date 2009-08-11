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

#ifndef PAGESPEED_CORE_PROTO_RESOURCE_UTILS_H_
#define PAGESPEED_CORE_PROTO_RESOURCE_UTILS_H_

#include <string>

namespace pagespeed {

class ProtoResource;

/**
 * Provides static helper methods that simplify ProtoResource creation
 * and manipulation.
 */
class ProtoResourceUtils {
 public:
  // Adds a request header to the resource.
  static void AddRequestHeader(ProtoResource* resource,
                               const std::string& key,
                               const std::string& value);

  // Adds a response header to the resource.
  static void AddResponseHeader(ProtoResource* resource,
                                const std::string& key,
                                const std::string& value);
};

}  // namespace pagespeed

#endif  // PAGESPEED_CORE_PROTO_RESOURCE_UTILS_H_
