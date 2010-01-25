/**
 * Copyright 2008-2009 Google Inc.
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

// Author: Bryan McQuade
//
// Implementation of CheckImpl for use in gecko binaries.

#include "check.h"

// Mozilla assertion APIs changed between gecko 1.8 and gecko 1.9.
#ifdef MOZILLA_1_8_BRANCH
#include "nsDebug.h"
#else
#include "nsXPCOM.h"
#endif

namespace activity {

void CheckImpl(const char *expr, const char *file, int line) {
#ifdef MOZILLA_1_8_BRANCH
  NSGlue_Assertion("Check failed: ", expr, file, line);
#else
  NS_DebugBreak(NS_DEBUG_ABORT, "Check failed: ", expr, file, line);
#endif
}

}  // namespace activity
