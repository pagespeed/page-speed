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

#include "base/scoped_ptr.h"
#include "base/logging.h"
#include "base/stl_util-inl.h"
#include "pagespeed/core/pagespeed_output.pb.h"
#include "pagespeed/core/rule.h"
#include "pagespeed/core/rule_registry.h"

namespace pagespeed {

Engine::Engine() {
}

bool Engine::GetResults(const PagespeedInput& input,
                        const Options& options,
                        Results* results) {
  std::vector<Rule*> rule_instances;
  RuleRegistry::CreateRuleInstances(options, &rule_instances);

  bool success = true;
  for (std::vector<Rule*>::const_iterator iter = rule_instances.begin(),
           end = rule_instances.end();
       iter != end;
       ++iter) {
    Rule* rule = *iter;
    success = rule->AppendResults(input, results) && success;
  }

  STLDeleteContainerPointers(rule_instances.begin(), rule_instances.end());

  return success;
}

}  // namespace pagespeed
