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

#ifndef PAGESPEED_CORE_RULE_H_
#define PAGESPEED_CORE_RULE_H_

#include <string>
#include <vector>
#include "base/basictypes.h"

namespace pagespeed {

class Formatter;
class InputInformation;
class PagespeedInput;
class Resource;
class Result;
class Results;
class ResultProvider;
class ResultText;

typedef std::vector<const Result*> ResultVector;

/**
 * Lint rule checker interface.
 */
class Rule {
 public:
  // RuleRequirements enumerates the types of input data that a Rule
  // instance may require. Certain types of data, such as response
  // headers and status code, are always required and thus not
  // enumerated here.
  enum RuleRequirements {
    NONE                       = 0,
    DOM                        = 1<<0,
    JS_CALLS_DOCUMENT_WRITE    = 1<<1,
    LAZY_LOADED                = 1<<2,
    PARENT_CHILD_RESOURCE_MAP  = 1<<3,
    REQUEST_HEADERS            = 1<<4,
    RESPONSE_BODY              = 1<<5,
    ALL                        = ~0,
  };

  explicit Rule(uint32 rule_requirements_bitfield);
  virtual ~Rule();

  // String that should be used to identify this rule during result
  // serialization.
  virtual const char* name() const = 0;

  // Human readable rule name.
  virtual const char* header() const = 0;

  // URL linking to the canonical documentation for this rule.
  virtual const char* documentation_url() const = 0;

  // A bitfield of RuleRequirements for this Rule.
  uint32 rule_requirements_bitfield() { return rule_requirements_bitfield_; }

  // Compute results and append it to the results set.
  //
  // @param input Input to process.
  // @param result_provider
  // @return true iff the computation was completed without errors.
  virtual bool AppendResults(const PagespeedInput& input,
                             ResultProvider* result_provider) = 0;

  // Interpret the results structure and produce a formatted representation.
  //
  // @param results Results to interpret
  // @param formatter Output formatter
  virtual void FormatResults(const ResultVector& results,
                             Formatter* formatter) = 0;

  // Compute the Rule score from InputInformation and ResultVector.
  //
  // @param input_info Information about resources that are part of the page.
  // @param results Result vector that contains savings information.
  // @returns 0-100 score.
  virtual int ComputeScore(const InputInformation& input_info,
                           const ResultVector& results);

  // Sort the results in their presentation order.
  virtual void SortResultsInPresentationOrder(ResultVector* rule_results) const;

private:
  const uint32 rule_requirements_bitfield_;
};

}  // namespace pagespeed

#endif  // PAGESPEED_CORE_RULE_H_
