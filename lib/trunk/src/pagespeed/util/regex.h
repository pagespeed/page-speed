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

#ifndef PAGESPEED_UTIL_REGEX_H_
#define PAGESPEED_UTIL_REGEX_H_

#ifdef _WIN32
#include <regex>
#else
#include <regex.h>
#endif

namespace pagespeed {

// Simple regex class that delegates to regex_t by default, and uses
// TR1 regex on Windows (where regex_t is unavailable).
class RE {
 public:
  explicit RE();
  ~RE();

  // Returns false if he pattterns is an invalid regex or if the RE
  // has already been initialized.
  bool Init(const char *pattern);

  bool is_valid() const { return is_valid_; }

  // Should not be called with an uninitialized or invalid RE.
  bool PartialMatch(const char *str) const;

private:
#ifdef _WIN32
  std::tr1::regex regex_;
#else
  regex_t regex_;
#endif
  bool is_initialized_;
  bool is_valid_;
};

}  // namespace pagespeed

#endif  // PAGESPEED_UTIL_REGEX_H_

