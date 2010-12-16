/**
 * Copyright 2009 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// Author: Matthew Steele

#ifndef PAGE_SPEED_RULES_H_
#define PAGE_SPEED_RULES_H_

#include "pagespeed_firefox/idl/IPageSpeedRules.h"

namespace pagespeed {

class PageSpeedRules : public IPageSpeedRules {
public:
  NS_DECL_ISUPPORTS
  NS_DECL_IPAGESPEEDRULES

  PageSpeedRules();

private:
  ~PageSpeedRules();
};

}  // namespace pagespeed

#endif  // PAGE_SPEED_RULES_H_
