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

#include "pagespeed/core/input_capabilities.h"

#include <string>

namespace pagespeed {

std::string InputCapabilities::DebugString() const {
  std::string has;
  std::string lacks;

  (capabilities_mask_ & DOM ? &has : &lacks)->append(" DOM");
  (capabilities_mask_ & ONLOAD ? &has : &lacks)->append(" ONLOAD");
  (capabilities_mask_ & REQUEST_HEADERS ?
   &has : &lacks)->append(" REQUEST_HEADERS");
  (capabilities_mask_ & RESPONSE_BODY ?
   &has : &lacks)->append(" RESPONSE_BODY");
  (capabilities_mask_ & REQUEST_START_TIMES ?
   &has : &lacks)->append(" REQUEST_START_TIMES");
  (capabilities_mask_ & TIMELINE_DATA ?
   &has : &lacks)->append(" TIMELINE_DATA");

  return "(Has:" + has + " ** Lacks:" + lacks + ")";
}

}  // namespace pagespeed
