// Copyright 2013 Google Inc. All Rights Reserved.
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

// Author: dbathgate@google.com (Daniel Bathgate)

#include "base/logging.h"
#include "pagespeed/core/rule.h"
#include "pagespeed/l10n/l10n.h"
#include "pagespeed/l10n/user_facing_string.h"

namespace pagespeed {

UserFacingString ExperimentalUserFacingString(Rule* rule, const char* s) {
  if (!rule->IsExperimental()) {
    LOG(DFATAL) << "Non-finalized translatable string used in non-experimental "
                << "rule! Replace non_finalized() with _() so this user facing "
                << "string can be localized.";
  }
  return not_localized(s);
}

} // namespace pagespeed
