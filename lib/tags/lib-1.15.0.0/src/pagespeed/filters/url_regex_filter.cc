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

#include "pagespeed/filters/url_regex_filter.h"

#include <string>
#include "pagespeed/core/resource.h"

namespace pagespeed {

UrlRegexFilter::UrlRegexFilter(const char* url_regex) {
  url_regex_.Init(url_regex);
}

UrlRegexFilter::~UrlRegexFilter() {}

bool UrlRegexFilter::IsAccepted(const Resource& resource) const {
  if (!url_regex_.is_valid()) {
    return false;
  }
  std::string url = resource.GetRequestUrl();
  return !url_regex_.PartialMatch(url.c_str());
}

}  // namespace pagespeed
