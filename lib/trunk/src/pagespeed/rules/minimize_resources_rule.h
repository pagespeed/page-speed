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

#ifndef PAGESPEED_RULES_MINIMIZE_RESOURCES_RULE_H_
#define PAGESPEED_RULES_MINIMIZE_RESOURCES_RULE_H_

#include "pagespeed/core/resource.h"
#include "pagespeed/core/rule.h"

namespace pagespeed {

/**
 * Checks for multiple, combinable resources of the same type served
 * off the same domain.
 */
class MinimizeResourcesRule : public Rule {
 protected:
  MinimizeResourcesRule(const char* rule_name, ResourceType resource_type);

 public:
  // Rule interface.
  virtual bool AppendResults(const PagespeedInput& input, Results* results);
  virtual void FormatResults(const Results& results, Formatter* formatter);

 private:
  const char* rule_name_;
  const ResourceType resource_type_;
  DISALLOW_COPY_AND_ASSIGN(MinimizeResourcesRule);
};

class MinimizeJsResourcesRule : public MinimizeResourcesRule {
 public:
  MinimizeJsResourcesRule();

 private:
  DISALLOW_COPY_AND_ASSIGN(MinimizeJsResourcesRule);
};

class MinimizeCssResourcesRule : public MinimizeResourcesRule {
 public:
  MinimizeCssResourcesRule();

 private:
  DISALLOW_COPY_AND_ASSIGN(MinimizeCssResourcesRule);
};

}  // namespace pagespeed

#endif  // PAGESPEED_RULES_MINIMIZE_RESOURCES_RULE_H_
