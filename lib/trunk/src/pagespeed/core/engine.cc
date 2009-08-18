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

#include "pagespeed/core/engine.h"

#include "base/stl_util-inl.h"  // for STLDeleteContainerPointers
#include "pagespeed/core/pagespeed_output.pb.h"
#include "pagespeed/core/rule.h"

namespace pagespeed {

Engine::Engine(const std::vector<Rule*>& rules) : rules_(rules) {
}

Engine::~Engine() {
  STLDeleteContainerPointers(rules_.begin(), rules_.end());
}

bool Engine::GetResults(const PagespeedInput& input, Results* results) {
  bool success = true;
  for (std::vector<Rule*>::const_iterator iter = rules_.begin(),
           end = rules_.end();
       iter != end;
       ++iter) {
    Rule* rule = *iter;
    success = rule->AppendResults(input, results) && success;
  }

  return success;
}

}  // namespace pagespeed
