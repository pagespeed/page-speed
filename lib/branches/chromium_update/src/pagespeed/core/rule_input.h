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

#include "base/basictypes.h"

namespace pagespeed {

class PagespeedInput;

class RuleInput {
 public:
  explicit RuleInput(const PagespeedInput& pagespeed_input)
      : pagespeed_input_(&pagespeed_input) {}

  const PagespeedInput& pagespeed_input() const { return *pagespeed_input_; }

  // TODO: Add more fields.

 private:
  const PagespeedInput* pagespeed_input_;

  DISALLOW_COPY_AND_ASSIGN(RuleInput);
};

}  // namespace pagespeed

#endif  // PAGESPEED_CORE_RULE_INPUT_H_
