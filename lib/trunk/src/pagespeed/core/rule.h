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

#ifndef PAGESPEED_CORE_RULE_H_
#define PAGESPEED_CORE_RULE_H_

#include <vector>

namespace pagespeed {

class PagespeedInput;
class Resource;
class Results;

/**
 * Lint rule checker interface.
 */
class Rule {
 public:
  Rule();
  virtual ~Rule();

  // Compute results and append it to the results set.
  //
  // @param input Input to process.
  // @param results
  // @return true iff the computation was completed without errors.
  virtual bool AppendResults(const PagespeedInput& input, Results* results) = 0;
};

}  // namespace pagespeed

#endif  // PAGESPEED_CORE_RULE_H_
