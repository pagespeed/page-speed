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

#include <algorithm> // for stable_sort
#include <string>

#include "base/logging.h"
#include "base/stl_util-inl.h"  // for STLDeleteContainerPointers
#include "pagespeed/core/formatter.h"
#include "pagespeed/core/pagespeed_input.h"
#include "pagespeed/core/resource.h"
#include "pagespeed/core/rule.h"
#include "pagespeed/proto/pagespeed_output.pb.h"

namespace pagespeed {

namespace {

/* Return true if result1 is judged to have (strictly) greater impact than
 * result2, false otherwise.  Note that this function imposes a total order on
 * what is essentially partially-ordered data, and thus gives somewhat
 * arbitrary answers. */
bool CompareResults(const Result* result1, const Result* result2) {
  const Savings& savings1 = result1->savings();
  const Savings& savings2 = result2->savings();
  if (savings1.dns_requests_saved() != savings2.dns_requests_saved()) {
    return savings1.dns_requests_saved() > savings2.dns_requests_saved();
  } else if (savings1.requests_saved() != savings2.requests_saved()) {
    return savings1.requests_saved() > savings2.requests_saved();
  } else {
    return savings1.response_bytes_saved() > savings2.response_bytes_saved();
  }
}

}  // namespace

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
    if (name_to_rule_map_.find(rule->name()) != name_to_rule_map_.end()) {
      LOG(DFATAL) << "Found duplicate rule while populating name to rule map.  "
                  << rule->name();
    }
    name_to_rule_map_[rule->name()] = rule;
  }
}

bool Engine::ComputeResults(const PagespeedInput& input,
                            Results* results) const {
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

bool Engine::FormatResults(const Results& results,
                           const InputInformation& input_info,
                           RuleFormatter* formatter) const {
  CHECK(init_);

  typedef std::map<std::string, ResultVector> RuleToResultMap;
  RuleToResultMap rule_to_result_map;

  for (int idx = 0, end = results.results_size(); idx < end; ++idx) {
    const Result& result = results.results(idx);
    rule_to_result_map[result.rule_name()].push_back(&result);
  }

  bool success = true;
  for (RuleToResultMap::const_iterator iter = rule_to_result_map.begin(),
           end = rule_to_result_map.end();
       iter != end;
       ++iter) {
    if (name_to_rule_map_.find(iter->first) == name_to_rule_map_.end()) {
      // No rule registered to handle the given rule name. This could
      // happen if the Results object was generated with a different
      // version of the Page Speed library, so we do not want to CHECK
      // that the Rule is non-null here.
      LOG(WARNING) << "Unable to find rule instance with name " << iter->first;
      success = false;
      continue;
    }
  }

  for (NameToRuleMap::const_iterator iter = name_to_rule_map_.begin(),
           end = name_to_rule_map_.end();
       iter != end;
       ++iter) {
    Rule* rule = iter->second;

    ResultVector& rule_results = rule_to_result_map[iter->first];
    std::stable_sort(rule_results.begin(), rule_results.end(), CompareResults);

    int score = rule->ComputeScore(input_info, rule_results);
    Formatter* rule_formatter = formatter->AddHeader(rule->header(), score);
    rule->FormatResults(rule_results, rule_formatter);
  }
  formatter->Done();

  return success;
}

bool Engine::ComputeAndFormatResults(const PagespeedInput& input,
                                     RuleFormatter* formatter) const {
  CHECK(init_);

  Results results;
  bool success = ComputeResults(input, &results);

  const InputInformation* input_info = input.input_information();
  success = FormatResults(results, *input_info, formatter) && success;
  return success;
}

}  // namespace pagespeed
