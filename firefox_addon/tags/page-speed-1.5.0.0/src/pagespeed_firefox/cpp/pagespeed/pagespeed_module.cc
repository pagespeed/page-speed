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

#include "activity/http_activity_distributor.h"
#include "activity/profiler.h"
#include "image_compressor/image_compressor.h"
#include "js_min/js_minifier.h"
#ifdef PAGESPEED_INCLUDE_LIBRARY_RULES
#include "pagespeed/pagespeed_rules.h"
#endif

#include "nsIGenericFactory.h"

namespace pagespeed {
#ifdef PAGESPEED_INCLUDE_LIBRARY_RULES
NS_GENERIC_FACTORY_CONSTRUCTOR(PageSpeedRules)
#endif
NS_GENERIC_FACTORY_CONSTRUCTOR(ImageCompressor)
NS_GENERIC_FACTORY_CONSTRUCTOR(JsMinifier)
}  // namespace pagespeed

namespace activity {
NS_GENERIC_FACTORY_CONSTRUCTOR(Profiler)
NS_GENERIC_FACTORY_CONSTRUCTOR(HttpActivityDistributor)
}  // namespace pagespeed

namespace {

#ifdef PAGESPEED_INCLUDE_LIBRARY_RULES
const char *PAGE_SPEED_RULES_CONTRACTID =
    "@code.google.com/p/page-speed/PageSpeedRules;1";
const char *PAGE_SPEED_RULES_CLASSNAME = "PageSpeedRules";
#endif

const char *IMAGE_COMPRESSOR_CONTRACTID =
    "@code.google.com/p/page-speed/ImageCompressor;1";
const char *IMAGE_COMPRESSOR_CLASSNAME = "ImageCompressor";

const char *JS_MINIFIER_CONTRACTID =
    "@code.google.com/p/page-speed/JsMin;1";
const char *JS_MINIFIER_CLASSNAME = "JsMinifier";

const char* PROFILER_CONTRACTID =
    "@code.google.com/p/page-speed/ActivityProfiler;1";
const char* PROFILER_CLASSNAME = "JavaScript Execution Tracer";
const char* HTTP_ACTIVITY_DISTRIBUTOR_CLASSNAME = "HTTP Activity Distributor";

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

#define IMAGE_COMPRESSOR_CID                          \
{ /* 972a3d18-1fed-4bfe-9465-fd419354233e */          \
  0x972a3d18,                                         \
  0x1fed,                                             \
  0x4bfe,                                             \
  { 0x94, 0x65, 0xfd, 0x41, 0x93, 0x54, 0x23, 0x3e }  \
}

#define JS_MINIFIER_CID                               \
{ /* 9e97eb52-2bea-4f77-9aa4-6eb2664db987 */          \
  0x9e97eb52,                                         \
  0x2bea,                                             \
  0x4f77,                                             \
  { 0x9a, 0xa4, 0x6e, 0xb26, 0x64, 0xd, 0xb9, 0x87 }  \
}

#define PROFILER_CID                                     \
{ /* 89cdb437-83b9-4544-ae85-7fb152458885 */             \
     0x89cdb437,                                         \
     0x83b9,                                             \
     0x4544,                                             \
     { 0xae, 0x85, 0x7f, 0xb1, 0x52, 0x45, 0x88, 0x85 }  \
}

#define HTTP_ACTIVITY_DISTRIBUTOR_CID                    \
{ /* 7ac44f14-2869-4a8f-b4e4-d88a51c10fc8 */             \
     0x7ac44f14,                                         \
     0x2869,                                             \
     0x4a8f,                                             \
     { 0xb4, 0xe4, 0xd8, 0x8a, 0x51, 0xc1, 0x0f, 0xc8 }  \
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
    IMAGE_COMPRESSOR_CLASSNAME,
    IMAGE_COMPRESSOR_CID,
    IMAGE_COMPRESSOR_CONTRACTID,
    pagespeed::ImageCompressorConstructor,
  },
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
  {
    HTTP_ACTIVITY_DISTRIBUTOR_CLASSNAME,
    HTTP_ACTIVITY_DISTRIBUTOR_CID,
    NS_HTTPACTIVITYDISTRIBUTOR_CONTRACTID,
    activity::HttpActivityDistributorConstructor,
  },
};

NS_IMPL_NSGETMODULE(PageSpeedModule, components)

}  // namespace
