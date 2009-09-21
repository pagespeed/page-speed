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

#include <string>

#include "base/logging.h"
#include "base/stl_util-inl.h"  // for STLDeleteContainerPointers
#include "pagespeed/core/rule.h"
#include "pagespeed/proto/pagespeed_output.pb.h"

namespace pagespeed {

Engine::Engine(const std::vector<Rule*>& rules) : rules_(rules), init_(false) {
}

Engine::~Engine() {
  STLDeleteContainerPointers(rules_.begin(), rules_.end());
}

void Engine::Init() {
  CHECK(!init_);

  PopulateNameToRuleMap();
  init_ = true;
}

void Engine::PopulateNameToRuleMap() {
  for (std::vector<Rule*>::const_iterator iter = rules_.begin(),
           end = rules_.end();
       iter != end;
       ++iter) {
    Rule *rule = *iter;
    CHECK(name_to_rule_map_.find(rule->name()) == name_to_rule_map_.end());
    name_to_rule_map_[rule->name()] = rule;
  }
}

bool Engine::ComputeResults(const PagespeedInput& input, Results* results) {
  CHECK(init_);

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

bool Engine::FormatResults(const Results& results, Formatter* formatter) {
  CHECK(init_);

  typedef std::map<std::string, std::vector<const Result*> > RuleToResultMap;

  bool success = true;
  RuleToResultMap rule_to_result_map;
  for (int idx = 0, end = results.results_size(); idx < end; ++idx) {
    const Result& result = results.results(idx);
    rule_to_result_map[result.rule_name()].push_back(&result);
  }

  for (RuleToResultMap::const_iterator iter = rule_to_result_map.begin(),
           end = rule_to_result_map.end();
       iter != end;
       ++iter) {
    Rule* rule = name_to_rule_map_[iter->first];
    if (!rule) {
      // No rule registered to handle the given rule name. This could
      // happen if the Results object was generated with a different
      // version of the Page Speed library, so we do not want to CHECK
      // that the Rule is non-null here.
      LOG(WARNING) << "Unable to find rule instance with name " << iter->first;
      success = false;
      continue;
    }
    const std::vector<const Result*>& rule_results = iter->second;
    rule->FormatResults(rule_results, formatter);
  }

  return success;
}

bool Engine::ComputeAndFormatResults(const PagespeedInput& input,
                                     Formatter* formatter) {
  CHECK(init_);

  Results results;
  bool success = ComputeResults(input, &results);
  success = FormatResults(results, formatter) && success;
  return success;
}

}  // namespace pagespeed
