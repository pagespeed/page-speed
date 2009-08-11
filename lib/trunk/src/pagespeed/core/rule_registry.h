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

#ifndef PAGESPEED_CORE_RULE_REGISTRY_H_
#define PAGESPEED_CORE_RULE_REGISTRY_H_

#include <map>
#include <string>
#include <vector>

#include "base/basictypes.h"
#include "base/singleton.h"

namespace pagespeed {

class Options;
class Rule;

// RuleFactory interface implemented by RuleFactoryTmpl below.
//
// Rule Factories must be thread-safe and, preferably stateless.
// The RuleFactory objects are shared among rule engines running in
// different threads so concurrent calls to New() are possible.
// Also, Rule constructors must be thread-safe.
class RuleFactory {
 public:
  virtual ~RuleFactory();
  virtual Rule* New() = 0;
};

// Generic, single implementation of the RuleFactory interface, which
// creates new rule object of the template type.
// Template type must implement the Rule interface.
template <class T>
class RuleFactoryTmpl : public RuleFactory {
 public:
  virtual Rule* New() {
    return new T;
  }
};

class RuleRegistry {
  typedef std::map<std::string, pagespeed::RuleFactory*> FactoryMap;
  friend class DefaultSingletonTraits<RuleRegistry>;

 public:
  // Create a rule factory and associates it with the given rule id.
  // This method should only be used in module initializers for rule
  // checker classes.  See REGISTER_RULE in rule.h for more details.
  template <class T>
  static void Register(const std::string& name) {
    RegisterImpl(name, new RuleFactoryTmpl<T>);
  }

  static void CreateRuleInstances(const Options& options,
                                  std::vector<Rule*>* rule_instances);

  // Freeze the registry, which prevents any further registrations and
  // allows access registered rule factories.  Called during module
  // initialization after all rules have been registered.
  static void Freeze();

 private:
  // For singleton creation
  RuleRegistry();

  pagespeed::Rule* New(const std::string& name) const;
  static void RegisterImpl(const std::string& name, RuleFactory* factory);

  bool frozen_;
  FactoryMap factories_;

  DISALLOW_COPY_AND_ASSIGN(RuleRegistry);
};

template<class T>
class RuleRegistration {
 public:
  explicit RuleRegistration(const std::string& name) {
    RuleRegistry::Register<T>(name);
  }
};

}  // namespace pagespeed

// Registers a lint checker rule implementation with the global rule
// registry during the InitGoogle module registration phase.
#define REGISTER_PAGESPEED_RULE(rule) \
  ::pagespeed::RuleRegistration<rule> pagespeed_ ## rule ## _registry(#rule)

#endif  // PAGESPEED_CORE_RULE_REGISTRY_H_
