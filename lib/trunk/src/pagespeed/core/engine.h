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

// Pagespeed rule engine.
//
// This API allows clients to query the library for rule violations
// triggered by the resources in the input set.

#ifndef PAGESPEED_CORE_ENGINE_H_
#define PAGESPEED_CORE_ENGINE_H_

#include <map>
#include <string>
#include <vector>

#include "base/basictypes.h"
#include "base/memory/scoped_ptr.h"

namespace pagespeed {

class Formatter;
class InputInformation;
class PagespeedInput;
class ResultText;
class Results;
class Result;
class Rule;
class RuleResults;

// ResultFilter is used to filter the results passed to the
// formatter. A ResultFilter might want to remove Results that have an
// impact under a certain threshold (e.g. saves less than 100 bytes).
class ResultFilter {
 public:
  ResultFilter();
  virtual ~ResultFilter();
  // Whether to retain the given Result instance.
  virtual bool IsResultAccepted(const Result& result) const = 0;

  // Whether to retain the given RuleResults instance. If false, the
  // entire RuleResults instance and all its child Results will be
  // discarded. If true, the RuleResults instance will be retained and
  // all its child Results instances will be passed to
  // IsResultAccepted(Result) to determine if they should be retained.
  virtual bool IsRuleResultsAccepted(const RuleResults& results) const = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(ResultFilter);
};

class AlwaysAcceptResultFilter : public ResultFilter {
 public:
  AlwaysAcceptResultFilter();
  virtual ~AlwaysAcceptResultFilter();

  virtual bool IsResultAccepted(const Result& result) const;
  virtual bool IsRuleResultsAccepted(const RuleResults& results) const;

 private:
  DISALLOW_COPY_AND_ASSIGN(AlwaysAcceptResultFilter);
};


// A ResultFilter that ANDs the result of two or more filters.
class AndResultFilter : public ResultFilter {
 public:
  // AndResultFilter takes ownership of the passed filters.
  AndResultFilter(ResultFilter* filter1, ResultFilter* filter2)
      : filter1_(filter1), filter2_(filter2) {}
  virtual ~AndResultFilter() {}

  virtual bool IsResultAccepted(const Result& result) const;
  virtual bool IsRuleResultsAccepted(const RuleResults& results) const;

 private:
  scoped_ptr<ResultFilter> filter1_;
  scoped_ptr<ResultFilter> filter2_;
};

class Engine {
 public:
  // Instantiate an Engine that uses the given Rule
  // instances. Ownership of the Rule instances is transferred to
  // the Engine object. The passed-in rules vector will be cleared
  // after the rule ownership is transferred to this Engine object.
  explicit Engine(std::vector<Rule*>* rules);
  virtual ~Engine();

  // Initialize the engine. Must be called once, immediately after
  // instantiating the engine.
  void Init();

  // Compute and add results to the result set by querying rule
  // objects about results they produce.
  // @return true iff the computation was completed without errors.
  bool ComputeResults(const PagespeedInput& input, Results* results) const;

  // Generate a formatted representation of the results, such as
  // human-readable markup that will be displayed to a user.
  // @return true iff the formatting was completed without errors.
  bool FormatResults(const Results& results,
                     const ResultFilter& filter,
                     Formatter* formatter) const;

  // Compute the results and generate their formatted
  // representation. This is a convenience method that invokes both
  // ComputeResults and FormatResults.
  // @return true iff the computation was completed without errors. if
  // false is returned, the formatter will only be invoked for those
  // results that did not generate errors.
  bool ComputeAndFormatResults(const PagespeedInput& input,
                               const ResultFilter& filter,
                               Formatter* formatter) const;

  bool FormatResults(const Results& results,
                     Formatter* formatter) const {
    AlwaysAcceptResultFilter filter;
    return FormatResults(results, filter, formatter);
  }

  bool ComputeAndFormatResults(const PagespeedInput& input,
                               Formatter* formatter) const {
    AlwaysAcceptResultFilter filter;
    return ComputeAndFormatResults(input, filter, formatter);
  }

  // Filters the results with the given filter into a second results proto,
  // and recomputing the impact for the filtered results.
  void FilterResults(const Results& results,
                     const ResultFilter& filter,
                     Results* filtered_results_out) const;

 private:
  // Computes the impact for each rule, as well as the overall score.
  // The given results should be as generated by ComputeResults
  // (and potentially filtered or manipulated thereafter).
  // @return true iff the computation was completed without errors.
  bool ComputeScoreAndImpact(Results* results) const;

  void PopulateNameToRuleMap();

  typedef std::map<std::string, Rule*> NameToRuleMap;

  std::vector<Rule*> rules_;
  NameToRuleMap name_to_rule_map_;
  bool init_has_been_called_;

  DISALLOW_COPY_AND_ASSIGN(Engine);
};

}  // namespace pagespeed

#endif  // PAGESPEED_CORE_ENGINE_H_
