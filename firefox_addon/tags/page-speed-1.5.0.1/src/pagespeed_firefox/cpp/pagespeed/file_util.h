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

#include <string>

#include "googleurl/src/gurl.h"

namespace pagespeed {

// Choose a filename at which to save data, given the URL associated with the
// data, and a string hash of the data.
std::string ChooseOutputFilename(const GURL& url, const std::string& hash);

}  // namespace pagespeed
