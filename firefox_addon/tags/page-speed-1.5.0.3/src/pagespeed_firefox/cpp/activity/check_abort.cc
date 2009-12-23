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

// Author: Bryan McQuade
//
// Implementation of CheckImpl for use in non-gecko binaries.

#include "check.h"

#include <stdio.h>
#include <stdlib.h>

namespace activity {

void CheckImpl(const char *expr, const char *file, int line) {
  fprintf(stderr, "Check failed: %s (%s:%d)\n", expr, file, line);
  exit(-1);
}

}  // namespace activity
