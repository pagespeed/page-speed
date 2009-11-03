// Copyright 2009 Google Inc.
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

#include "pagespeed_webkit/util.h"

#include "CString.h"
#include "PlatformString.h"
#include "TextEncoding.h"

namespace pagespeed_webkit {

std::string ToAscii(const WebCore::String& string) {
  const WebCore::TextEncoding& ascii = WebCore::ASCIIEncoding();

  const WebCore::CString& encoded =
      ascii.encode(string.characters(),
                   string.length(),
                   WebCore::URLEncodedEntitiesForUnencodables);

  return std::string(encoded.data(), encoded.length());
}

}  // namespace pagespeed_webkit
