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

#ifndef PAGESPEED_JSMINIFY_JS_MINIFY_H_
#define PAGESPEED_JSMINIFY_JS_MINIFY_H_

#include <string>

#include "base/string_piece.h"

namespace pagespeed {

namespace jsminify {

// Return true if minification was successful, false otherwise.
bool MinifyJs(const base::StringPiece& input, std::string* out);

// Return true if minification was successful, false otherwise.
bool GetMinifiedJsSize(const base::StringPiece& input, int* minimized_size);

}  // namespace jsminify

}  // namespace pagespeed

#endif  // PAGESPEED_JSMINIFY_JS_MINIFY_H_
