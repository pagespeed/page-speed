// Copyright 2010 Google Inc.
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

#include <algorithm>
#include <string>

#include "base/logging.h"
#include "base/stl_util-inl.h"  // for STLDeleteContainerPointers
#include "pagespeed/core/formatter.h"
#include "pagespeed/core/pagespeed_input.h"
#include "pagespeed/core/pagespeed_version.h"
#include "pagespeed/core/resource.h"
#include "pagespeed/core/resource_util.h"
#include "pagespeed/core/result_provider.h"
#include "pagespeed/core/rule.h"
#include "pagespeed/core/rule_input.h"
#include "pagespeed/proto/pagespeed_output.pb.h"

namespace pagespeed {

namespace {

void FormatRuleResults(const RuleResults& rule_results,
                       const InputInformation& input_info,
                       Rule* rule,
                       const ResultFilter& filter,
                       Formatter* root_formatter) {
  // Sort results according to presentation order
  ResultVector sorted_results;
  for (int result_idx = 0, end = rule_results.results_size();
       result_idx < end; ++result_idx) {
    const Result& result = rule_results.results(result_idx);

    if (filter.IsAccepted(result)) {
      sorted_results.push_back(&rule_results.results(result_idx));
    }
  }
  rule->SortResultsInPresentationOrder(&sorted_results);

  RuleFormatter* rule_formatter =
      root_formatter->AddRule(*rule, rule_results.rule_score(),
                              rule_results.rule_impact());
  if (!sorted_results.empty()) {
    rule->FormatResults(sorted_results, rule_formatter);
  }
}

double ComputePageWeight(const InputInformation& input_info) {
  const ClientCharacteristics& client = input_info.client_characteristics();
  double weight = 0.0;
  weight += client.dns_requests_weight() * input_info.number_hosts();
  weight += client.requests_weight() * input_info.number_resources();
  weight += client.response_bytes_weight() *
      resource_util::ComputeTotalResponseBytes(input_info);
  weight += client.request_bytes_weight() * input_info.total_request_bytes();

  // There are at least as many connections as there are hosts.
  // TODO(mdsteele): revisit this when we know the exact number of
  // connections.
  weight += client.connections_weight() + input_info.number_hosts();

  // TODO(mdsteele): Note that there are some fields of ClientCharacteristics
  // that we don't use here yet:
  //
  // - page_reflows_weight: There's no good way for us to know how many times
  //   the page reflowed while rendering, is there?
  //
  // - critical_path_length_weight: We should multiply this by the current
  //   length of the page's critical path; do we have of knowing that?
  //
  // - expected_cache_hit_rate: Maybe we should be multiplying the requests
  //   term above by (1 - hit_rate)?
  return weight;
}

}  // namespace

Engine::Engine(std::vector<Rule*>* rules)
    : rules_(*rules), init_has_been_called_(false) {
  // Now that we've transferred the rule ownership to our local
  // vector, clear the passed in vector.
  rules->clear();
}

Engine::~Engine() {
  STLDeleteContainerPointers(rules_.begin(), rules_.end());
}

void Engine::Init() {
  CHECK(!init_has_been_called_);

  PopulateNameToRuleMap();
  init_has_been_called_ = true;
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

bool Engine::ComputeResults(const PagespeedInput& pagespeed_input,
                            Results* results) const {
  CHECK(init_has_been_called_);

  if (!pagespeed_input.is_frozen()) {
    LOG(DFATAL) << "Attempting to ComputeResults with non-frozen input.";
    return false;
  }

  results->mutable_input_info()->CopyFrom(*pagespeed_input.input_information());
  GetPageSpeedVersion(results->mutable_version());

  RuleInput rule_input(pagespeed_input);
  rule_input.Init();
  int num_results_so_far = 0;

  bool success = true;
  for (std::vector<Rule*>::const_iterator iter = rules_.begin(),
           end = rules_.end();
       iter != end;
       ++iter) {
    Rule* rule = *iter;
    RuleResults* rule_results = results->add_rule_results();
    rule_results->set_rule_name(rule->name());

    ResultProvider provider(*rule, rule_results, num_results_so_far);
    const bool rule_success = rule->AppendResults(rule_input, &provider);
    num_results_so_far += provider.num_new_results();
    if (!rule_success) {
      // Record that the rule encountered an error.
      results->add_error_rules(rule->name());
      success = false;
    }
  }

  if (!ComputeScoreAndImpact(results)) {
    success = false;
  }

  if (!results->IsInitialized()) {
    LOG(DFATAL) << "Failed to fully initialize results object.";
    return false;
  }

  return success;
}

bool Engine::FormatResults(const Results& results,
                           const ResultFilter& filter,
                           Formatter* formatter) const {
  CHECK(init_has_been_called_);

  if (!results.IsInitialized()) {
    LOG(ERROR) << "Results instance not fully initialized.";
    return false;
  }

  bool success = true;
  for (int idx = 0, end = results.rule_results_size(); idx < end; ++idx) {
    const RuleResults& rule_results = results.rule_results(idx);
    const std::string& rule_name = rule_results.rule_name();
    NameToRuleMap::const_iterator rule_iter = name_to_rule_map_.find(rule_name);
    if (rule_iter == name_to_rule_map_.end()) {
      // No rule registered to handle the given rule name. This could
      // happen if the Results object was generated with a different
      // version of the Page Speed library, so we do not want to CHECK
      // that the Rule is non-null here.
      LOG(WARNING) << "Unable to find rule instance with name " << rule_name;
      success = false;
      continue;
    }

    Rule* rule = rule_iter->second;
    FormatRuleResults(rule_results, results.input_info(), rule, filter,
                      formatter);
  }

  if (results.has_score()) {
    formatter->SetOverallScore(results.score());
  }
  formatter->Finalize();

  return success;
}

bool Engine::ComputeAndFormatResults(const PagespeedInput& input,
                                     const ResultFilter& filter,
                                     Formatter* formatter) const {
  CHECK(init_has_been_called_);

  Results results;
  bool success = ComputeResults(input, &results);
  success = FormatResults(results, filter, formatter) && success;
  return success;
}

bool Engine::ComputeScoreAndImpact(Results* results) const {
  CHECK(init_has_been_called_);

  double total_impact = 0.0;
  bool any_rules_succeeded = false;

  bool success = true;
  for (int i = 0; i < results->rule_results_size(); ++i) {
    RuleResults* rule_results = results->mutable_rule_results(i);
    rule_results->clear_rule_score();
    rule_results->clear_rule_impact();

    const std::string& rule_name = rule_results->rule_name();
    NameToRuleMap::const_iterator rule_iter = name_to_rule_map_.find(rule_name);
    if (rule_iter == name_to_rule_map_.end()) {
      // No rule registered to handle the given rule name. This could
      // happen if the Results object was generated with a different
      // version of the Page Speed library, so we do not want to CHECK
      // that the Rule is non-null here.
      LOG(WARNING) << "Unable to find rule instance with name " << rule_name;
      success = false;
      continue;
    }

    Rule* rule = rule_iter->second;

    double impact = 0.0;
    if (rule_results->results_size() > 0) {
      impact = rule->ComputeRuleImpact(results->input_info(), *rule_results);
      if (impact < 0.0) {
        LOG(ERROR) << "Impact for " << rule->name() << " out of bounds: "
                   << impact;
        impact = 0.0;
      }
    }
    rule_results->set_rule_impact(impact);
    if (!rule->IsExperimental()) {
      total_impact += impact;
    }

    int score = 100;
    if (rule_results->results_size() > 0) {
      score = rule->ComputeScore(results->input_info(), *rule_results);
      if (score > 100 || score < -1) {
        // Note that the value -1 indicates a valid score could not be
        // computed, so we need to allow it.
        LOG(ERROR) << "Score for " << rule->name() << " out of bounds: "
                   << score;
        score = std::max(-1, std::min(100, score));
      }
    }

    // TODO(bmcquade): Ideally we would not set the rule score if
    // there was a rule error, however many of our rules generate
    // errors when they encounter invalid content (e.g. when we
    // encounter an image that we can't parse). These content errors
    // are not fatal errors and we may still be able to generate a
    // meaningful score in these cases. Once we fix rules to only
    // signal error on internal Page Speed logic errors, we can update
    // the if test below to also check for rule_success, in order to
    // prevent setting a score when we encounter an internal error.
    //
    // Instead of using a -1 to indicate an error, we just don't set
    // rule_score.
    if (score >= 0) {
      any_rules_succeeded = true;
      rule_results->set_rule_score(score);
    }
  }

  // Compute the overall score based on the impacts of the rules compared to
  // the current weight of the page.  Only set the overall score if at least
  // one rule ran successfully.
  // TODO(mdsteele): Ideally, we would be smarter than just summing the
  //   impacts.  For example, maybe the impact of rules A and B together is
  //   less than their sum (because they overlap), and maybe the impact of
  //   rules B and C together is greater than their sum (because they're
  //   synergetic).
  if (any_rules_succeeded) {
    DCHECK(total_impact >= 0.0);
    if (total_impact == 0.0) {
      // If the impact is zero, the score should be perfect (even if page
      // weight is zero).
      results->set_score(100);
    } else {
      const double page_weight = ComputePageWeight(results->input_info());
      DCHECK(page_weight >= 0.0);
      // If the impact is positive but the page weight is zero, we'll get a
      // score of max(0, -infinity) == 0, which is what we want.
      results->set_score(static_cast<int>(
          std::max(0.0, 100.0 * (1.0 - total_impact / page_weight))));
    }
  }

  return success;
}

void Engine::FilterResults(const Results& results,
                           const ResultFilter& filter,
                           Results* filtered_results_out) const {
  CHECK(init_has_been_called_);

  filtered_results_out->CopyFrom(results);

  for (int rule_idx = 0;
       rule_idx < filtered_results_out->rule_results_size();
       ++rule_idx) {
    RuleResults* rule_results =
        filtered_results_out->mutable_rule_results(rule_idx);
    RuleResults new_rule_results;

    // Copy any non-filtered results into new_rule_results;
    for (int result_idx = 0;
         result_idx < rule_results->results_size();
         ++result_idx) {
      const Result& result = rule_results->results(result_idx);

      if (filter.IsAccepted(result)) {
        new_rule_results.add_results()->CopyFrom(result);
      }
    }

    // Clear out the old results and copy back in the filtered set.
    rule_results->clear_results();
    rule_results->MergeFrom(new_rule_results);
  }

  ComputeScoreAndImpact(filtered_results_out);
}

ResultFilter::ResultFilter() {}
ResultFilter::~ResultFilter() {}

AlwaysAcceptResultFilter::AlwaysAcceptResultFilter() {}
AlwaysAcceptResultFilter::~AlwaysAcceptResultFilter() {}

bool AlwaysAcceptResultFilter::IsAccepted(const Result& result) const {
  return true;
}

}
