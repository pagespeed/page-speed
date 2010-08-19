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

#include "pagespeed/core/uri_util.h"

#include "googleurl/src/gurl.h"
#include "googleurl/src/url_canon.h"

namespace pagespeed {

std::string ResolveUri(const std::string& uri, const std::string& base_url) {
  GURL url(base_url);
  if (!url.is_valid()) {
    return "";
  }

  GURL derived = url.Resolve(uri);
  if (!derived.is_valid()) {
    return "";
  }

  // Remove everything after the #, which is not sent to the server,
  // and return the resulting url.
  //
  // TODO: this should probably not be the default behavior; user
  // should have to explicitly remove the fragment.
  url_canon::Replacements<char> clear_fragment;
  clear_fragment.ClearRef();
  return derived.ReplaceComponents(clear_fragment).spec();
}

}  // namespace pagespeed
