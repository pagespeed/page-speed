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

#ifndef PAGESPEED_RULES_RULE_PROVIDER_H_
#define PAGESPEED_RULES_RULE_PROVIDER_H_

#include <vector>

namespace pagespeed {

class Rule;

namespace rule_provider {

/**
 * Append the core Page Speed rules to the given vector of Rule
 * instances.
 */
void AppendCoreRules(std::vector<Rule*> *rules);

}  // namespace rule_provider

}  // namespace pagespeed

#endif  // PAGESPEED_RULES_RULE_PROVIDER_H_
