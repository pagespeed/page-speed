// Copyright 2011 Google Inc. All Rights Reserved.
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

#ifndef PAGESPEED_CSS_EXTERNAL_RESOURCE_FINDER_H_
#define PAGESPEED_CSS_EXTERNAL_RESOURCE_FINDER_H_

#include <set>
#include <string>

#include "base/basictypes.h"

namespace pagespeed {

class Resource;

namespace css {

// Finds resources referenced from the body of a CSS resource.
class ExternalResourceFinder {
 public:
  ExternalResourceFinder();

  // Scans the body of the given CSS resource and emits a list of
  // resource URLs referenced from the CSS resource.
  void FindExternalResources(const Resource& resource,
                             std::set<std::string>* external_resource_urls);

  // These methods are exposed only for unittesting. They should not
  // be called by non-test code.
  static void RemoveComments(const std::string& in, std::string* out);
  static bool IsCssImportLine(const std::string& line, std::string* out_url);

 private:
  DISALLOW_COPY_AND_ASSIGN(ExternalResourceFinder);
};

}  // namespace css
}  // namespace pagespeed



#endif  // PAGESPEED_CSS_EXTERNAL_RESOURCE_FINDER_H_
