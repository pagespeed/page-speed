// Copyright 2009 Google Inc.
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

// Pagespeed rule engine.
//
// This API allows clients to query the library for rule violations
// triggered by the resources in the input set.

#ifndef PAGESPEED_CORE_ENGINE_H_
#define PAGESPEED_CORE_ENGINE_H_

#include <vector>

#include "base/basictypes.h"

namespace pagespeed {

class Options;
class PagespeedInput;
class Results;

class Engine {
 public:
  Engine();

  // Compute and add results to the result set by querying rule
  // objects about results they produce.
  // @return true iff the computation was completed without errors.
  static bool GetResults(const PagespeedInput& input,
                         const Options& options,
                         Results* results);

  DISALLOW_COPY_AND_ASSIGN(Engine);
};

}  // namespace pagespeed

#endif  // PAGESPEED_CORE_ENGINE_H_
