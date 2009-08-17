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

#include "pagespeed/rules/rule_provider.h"

#include "pagespeed/rules/gzip_rule.h"
#include "pagespeed/rules/minimize_dns_rule.h"
#include "pagespeed/rules/minimize_resources_rule.h"

namespace pagespeed {

namespace rule_provider {

void AppendCoreRules(std::vector<Rule*> *rules) {
  rules->push_back(new GzipRule());
  rules->push_back(new MinimizeDnsRule());
  rules->push_back(new MinimizeJsResourcesRule());
  rules->push_back(new MinimizeCssResourcesRule());
}

}  // namespace rule_provider

}  // namespace pagespeed
