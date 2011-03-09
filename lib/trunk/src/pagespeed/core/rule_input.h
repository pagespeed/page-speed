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

#ifndef PAGESPEED_CORE_RULE_INPUT_H_
#define PAGESPEED_CORE_RULE_INPUT_H_

#include <map>
#include <string>
#include <vector>


#include "base/basictypes.h"

namespace pagespeed {

class PagespeedInput;

class RuleInput {
 public:
  typedef std::vector<const Resource*> RedirectChain;
  typedef std::vector<RedirectChain> RedirectChainVector;
  typedef std::map<const Resource*, const RedirectChain*>
      ResourceToRedirectChainMap;

  explicit RuleInput(const PagespeedInput& pagespeed_input)
      : pagespeed_input_(&pagespeed_input),
        initialized_(false) {}
  void Init();
  const PagespeedInput& pagespeed_input() const { return *pagespeed_input_; }
  const RedirectChainVector& GetRedirectChains() const;
  const RedirectChain* GetRedirectChainOrNull(const Resource* resource) const;
 private:
  void BuildRedirectChains();

  const PagespeedInput* pagespeed_input_;
  RedirectChainVector redirect_chains_;
  ResourceToRedirectChainMap resource_to_redirect_chain_map_;
  bool initialized_;

  DISALLOW_COPY_AND_ASSIGN(RuleInput);
};

}  // namespace pagespeed

#endif  // PAGESPEED_CORE_RULE_INPUT_H_
