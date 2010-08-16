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

// Author: Bryan McQuade
// Author: Tony Gentilcore

#include "pagespeed/pagespeed_rules.h"

#include "nsIGenericFactory.h"

namespace pagespeed {
NS_GENERIC_FACTORY_CONSTRUCTOR(PageSpeedRules)
}  // namespace pagespeed

namespace {

const char *PAGE_SPEED_RULES_CONTRACTID =
    "@code.google.com/p/page-speed/PageSpeedRules;1";
const char *PAGE_SPEED_RULES_CLASSNAME = "PageSpeedRules";

// CIDs, or "class identifiers", are used by xpcom to uniquely
// identify a class or component. See
// http://www.mozilla.org/projects/xpcom/book/cxc/html/quicktour2.html#1005329
// for more information.

#define PAGE_SPEED_RULES_CID                             \
{ /* 9d5c2098-b43c-4874-a12a-57c4b93896aa */             \
     0x9d5c2098,                                         \
     0xb43c,                                             \
     0x4874,                                             \
     { 0xa1, 0x2a, 0x57, 0xc4, 0xb9, 0x38, 0x96, 0xaa }  \
}

static nsModuleComponentInfo components[] = {
  {
    PAGE_SPEED_RULES_CLASSNAME,
    PAGE_SPEED_RULES_CID,
    PAGE_SPEED_RULES_CONTRACTID,
    pagespeed::PageSpeedRulesConstructor,
  },
};

NS_IMPL_NSGETMODULE(PageSpeedModule, components)

}  // namespace
