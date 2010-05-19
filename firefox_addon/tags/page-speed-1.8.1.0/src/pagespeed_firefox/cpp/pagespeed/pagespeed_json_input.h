/**
 * Copyright 2009 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// Author: Matthew Steele

#ifndef PAGESPEED_JSON_INPUT_H_
#define PAGESPEED_JSON_INPUT_H_

#include <string>
#include <vector>

#include "pagespeed/core/pagespeed_input.h"

namespace pagespeed {

// Parse the JSON string and use it to populate the input.  If any errors
// occur, log them and return false, otherwise return true.
bool PopulateInputFromJSON(PagespeedInput *input, const char *json_data,
                           const std::vector<std::string> &contents);

}  // namespace pagespeed

#endif  // PAGESPEED_JSON_INPUT_H_
