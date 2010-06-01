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

#include "activity/profiler.h"
#include "js_min/js_minifier.h"
#ifdef PAGESPEED_INCLUDE_LIBRARY_RULES
#include "pagespeed/pagespeed_rules.h"
#endif

#include "nsIGenericFactory.h"

namespace pagespeed {
#ifdef PAGESPEED_INCLUDE_LIBRARY_RULES
NS_GENERIC_FACTORY_CONSTRUCTOR(PageSpeedRules)
#endif
NS_GENERIC_FACTORY_CONSTRUCTOR(JsMinifier)
}  // namespace pagespeed

namespace activity {
NS_GENERIC_FACTORY_CONSTRUCTOR(Profiler)
}  // namespace pagespeed

namespace {

#ifdef PAGESPEED_INCLUDE_LIBRARY_RULES
const char *PAGE_SPEED_RULES_CONTRACTID =
    "@code.google.com/p/page-speed/PageSpeedRules;1";
const char *PAGE_SPEED_RULES_CLASSNAME = "PageSpeedRules";
#endif

const char *JS_MINIFIER_CONTRACTID =
    "@code.google.com/p/page-speed/JsMin;1";
const char *JS_MINIFIER_CLASSNAME = "JsMinifier";

const char* PROFILER_CONTRACTID =
    "@code.google.com/p/page-speed/ActivityProfiler;1";
const char* PROFILER_CLASSNAME = "JavaScript Execution Tracer";

// CIDs, or "class identifiers", are used by xpcom to uniquely
// identify a class or component. See
// http://www.mozilla.org/projects/xpcom/book/cxc/html/quicktour2.html#1005329
// for more information.

#ifdef PAGESPEED_INCLUDE_LIBRARY_RULES
#define PAGE_SPEED_RULES_CID                             \
{ /* 9d5c2098-b43c-4874-a12a-57c4b93896aa */             \
     0x9d5c2098,                                         \
     0xb43c,                                             \
     0x4874,                                             \
     { 0xa1, 0x2a, 0x57, 0xc4, 0xb9, 0x38, 0x96, 0xaa }  \
}
#endif

#define JS_MINIFIER_CID                               \
{ /* 9e97eb52-2bea-4f77-9aa4-6e26640db987 */          \
  0x9e97eb52,                                         \
  0x2bea,                                             \
  0x4f77,                                             \
  { 0x9a, 0xa4, 0x6e, 0x26, 0x64, 0x0d, 0xb9, 0x87 }  \
}

#define PROFILER_CID                                     \
{ /* 89cdb437-83b9-4544-ae85-7fb152458885 */             \
     0x89cdb437,                                         \
     0x83b9,                                             \
     0x4544,                                             \
     { 0xae, 0x85, 0x7f, 0xb1, 0x52, 0x45, 0x88, 0x85 }  \
}

static nsModuleComponentInfo components[] = {
#ifdef PAGESPEED_INCLUDE_LIBRARY_RULES
  {
    PAGE_SPEED_RULES_CLASSNAME,
    PAGE_SPEED_RULES_CID,
    PAGE_SPEED_RULES_CONTRACTID,
    pagespeed::PageSpeedRulesConstructor,
  },
#endif
  {
    JS_MINIFIER_CLASSNAME,
    JS_MINIFIER_CID,
    JS_MINIFIER_CONTRACTID,
    pagespeed::JsMinifierConstructor,
  },
  {
    PROFILER_CLASSNAME,
    PROFILER_CID,
    PROFILER_CONTRACTID,
    activity::ProfilerConstructor,
  },
};

NS_IMPL_NSGETMODULE(PageSpeedModule, components)

}  // namespace
