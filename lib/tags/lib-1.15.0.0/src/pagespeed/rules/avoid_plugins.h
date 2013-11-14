// Copyright 2012 Google Inc. All Rights Reserved.
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

#ifndef PAGESPEED_RULES_AVOID_PLUGINS_H_
#define PAGESPEED_RULES_AVOID_PLUGINS_H_

#include <vector>

#include "base/basictypes.h"
#include "pagespeed/core/rule.h"
#include "pagespeed/l10n/user_facing_string.h"
#include "pagespeed/proto/pagespeed_output.pb.h"

namespace pagespeed {

class UrlBlockFormatter;

namespace rules {

/**
 * Checks for flash objects in the resource list
 */
class AvoidPlugins : public Rule {
 public:
  AvoidPlugins();

  // Rule interface.
  virtual const char* name() const;
  virtual UserFacingString header() const;
  virtual bool AppendResults(const RuleInput& input, ResultProvider* provider);
  virtual void FormatResults(const ResultVector& results,
                             RuleFormatter* formatter);
  virtual void AppendRuleGroups(
      std::vector<FormattedRuleResults::RuleGroup>* out) const;
  virtual bool IsExperimental() const;

 protected:
  virtual double ComputeResultImpact(const InputInformation& input_info,
                                     const Result& result);

 private:
  UrlBlockFormatter* CreateUrlBlockFormatterForType(
      const AvoidPluginsDetails_PluginType& type, RuleFormatter* formatter);

  DISALLOW_COPY_AND_ASSIGN(AvoidPlugins);
};

}  // namespace rules

}  // namespace pagespeed

#endif  // PAGESPEED_RULES_AVOID_PLUGINS_H_
