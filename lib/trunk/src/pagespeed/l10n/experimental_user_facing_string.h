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

#ifndef PAGESPEED_L10N_EXPERIMENTAL_USER_FACING_STRING_H_
#define PAGESPEED_L10N_EXPERIMENTAL_USER_FACING_STRING_H_

#include "pagespeed/l10n/user_facing_string.h"

namespace pagespeed {

class Rule;

// Creates a non-translatable user facing string, enforcing that the provided
// rule is experimental. This should only be called by the not_localized macro
// in l10n.h.
UserFacingString ExperimentalUserFacingString(Rule* rule, const char* s);

} // namespace pagespeed

#endif  // PAGESPEED_L10N_EXPERIMENTAL_USER_FACING_STRING_H_
