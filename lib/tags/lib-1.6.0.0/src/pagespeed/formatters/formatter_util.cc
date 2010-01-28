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

#include "pagespeed/formatters/formatter_util.h"

#include "base/string_util.h"

namespace pagespeed {

namespace formatters {

const int kBytesPerKiB = 1 << 10;
const int kBytesPerMiB = 1 << 20;

std::string FormatBytes(const int bytes) {
  if (bytes < kBytesPerKiB) {
    return StringPrintf("%dB", bytes);
  } else if (bytes < kBytesPerMiB) {
    return StringPrintf("%.1fKiB", bytes / static_cast<double>(kBytesPerKiB));
  } else {
    return StringPrintf("%.1fMiB", bytes / static_cast<double>(kBytesPerMiB));
  }
}

}  // namespace formatters

}  // namespace pagespeed
