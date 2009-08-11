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

#include "pagespeed/core/rule_registry.h"

#include "base/logging.h"
#include "pagespeed/core/pagespeed_options.pb.h"
#include "pagespeed/core/rule.h"

namespace pagespeed {

RuleFactory::~RuleFactory() {
}

RuleRegistry::RuleRegistry() : frozen_(false) {
}

void RuleRegistry::CreateRuleInstances(const Options& options,
                                       std::vector<Rule*> *rule_instances) {
  CHECK(rule_instances != NULL);
  RuleRegistry* instance = Singleton<RuleRegistry>::get();
  CHECK(instance->frozen_)
      << "Tried to get a RuleFactory but the RuleRegistry is not frozen.  "
      << "Please call RuleRegistry::Freeze before instantiating rule engines.";

  typedef ::google::protobuf::RepeatedPtrField<std::string> RuleNameList;
  const RuleNameList& rule_names = options.rule_names();
  if (rule_names.size() > 0) {
    // Add selected rules
    for (RuleNameList::const_iterator iter = rule_names.begin(),
             end = rule_names.end();
         iter != end;
         ++iter) {
      Rule* rule = instance->New(*iter);
      DCHECK_NE(rule, static_cast<Rule*>(NULL));
      rule_instances->push_back(rule);
    }
  } else {
    // Add all rules
    for (FactoryMap::const_iterator iter = instance->factories_.begin(),
             end = instance->factories_.end();
         iter != end;
         ++iter) {
      Rule* rule = iter->second->New();
      DCHECK_NE(rule, static_cast<Rule*>(NULL));
      rule_instances->push_back(rule);
    }
  }
}

void RuleRegistry::Freeze() {
  RuleRegistry* instance = Singleton<RuleRegistry>::get();
  CHECK(!instance->frozen_) << "RuleRegistry::Freeze called multiple times.";
  instance->frozen_ = true;
}

pagespeed::Rule* RuleRegistry::New(const std::string& name) const {
  CHECK(frozen_)
      << "Tried to get a RuleFactory but the RuleRegistry is not frozen.  "
      << "Please call RuleRegistry::Freeze before instantiating rule engines.";
  FactoryMap::const_iterator iter = factories_.find(name);
  RuleFactory *rule_factory = (iter != factories_.end()) ? iter->second : NULL;
  CHECK(rule_factory != NULL) << "No handler for \"" << name << "\"";
  return rule_factory->New();
}

void RuleRegistry::RegisterImpl(const std::string& name, RuleFactory* factory) {
  RuleRegistry* instance = Singleton<RuleRegistry>::get();
  FactoryMap& factories = instance->factories_;
  CHECK(!instance->frozen_)
      << "Tried to register a rule but RuleRegistry is already frozen.  "
      << "Please use REGISTER_RULE for all rule registrations.";
  std::pair<FactoryMap::iterator, bool> status =
      factories.insert(std::make_pair(name, factory));
  CHECK(status.second);
}

}  // namespace pagespeed
