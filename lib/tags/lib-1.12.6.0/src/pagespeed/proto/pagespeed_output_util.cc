// Copyright 2011 Google Inc. All Rights Reserved.
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

#include "pagespeed/proto/pagespeed_output_util.h"

#include "pagespeed/proto/pagespeed_output.pb.h"

namespace pagespeed {

namespace proto {

bool AllResultsHaveIds(const pagespeed::Results& results) {
  for (int idx = 0, end = results.rule_results_size(); idx < end; ++idx) {
    const RuleResults& rule_results = results.rule_results(idx);
    for (int result_idx = 0, result_end = rule_results.results_size();
         result_idx < result_end; ++result_idx) {
      const Result& result = rule_results.results(result_idx);
      if (!result.has_id()) {
        return false;
      }
    }
  }
  return true;
}

void ClearResultIds(pagespeed::Results* results) {
  for (int idx = 0, end = results->rule_results_size(); idx < end; ++idx) {
    RuleResults* rule_results = results->mutable_rule_results(idx);
    for (int result_idx = 0, result_end = rule_results->results_size();
         result_idx < result_end; ++result_idx) {
      rule_results->mutable_results(result_idx)->clear_id();
    }
  }
}

bool PopulateResultIds(pagespeed::Results* results) {
  for (int idx = 0, end = results->rule_results_size(); idx < end; ++idx) {
    const RuleResults& rule_results = results->rule_results(idx);
    for (int result_idx = 0, result_end = rule_results.results_size();
         result_idx < result_end; ++result_idx) {
      const Result& result = rule_results.results(result_idx);
      if (result.has_id()) {
        return false;
      }
    }
  }

  int id = 0;
  for (int idx = 0, end = results->rule_results_size(); idx < end; ++idx) {
    RuleResults* rule_results = results->mutable_rule_results(idx);
    for (int result_idx = 0, result_end = rule_results->results_size();
         result_idx < result_end; ++result_idx) {
      Result* result = rule_results->mutable_results(result_idx);
      result->set_id(id++);
    }
  }
  return true;
}

}  // namespace proto

}  // namespace pagespeed
