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

#include "nsIGenericFactory.h"

namespace pagespeed {
NS_GENERIC_FACTORY_CONSTRUCTOR(ImageCompressor)
}  // namespace pagespeed

namespace activity {
NS_GENERIC_FACTORY_CONSTRUCTOR(Profiler)
NS_GENERIC_FACTORY_CONSTRUCTOR(HttpActivityDistributor)
}  // namespace pagespeed

namespace {

const char *IMAGE_COMPRESSOR_CONTRACTID =
    "@code.google.com/p/page-speed/ImageCompressor;1";
const char *IMAGE_COMPRESSOR_CLASSNAME = "ImageCompressor";

const char* PROFILER_CONTRACTID =
    "@code.google.com/p/page-speed/ActivityProfiler;1";
const char* PROFILER_CLASSNAME = "JavaScript Execution Tracer";
const char* HTTP_ACTIVITY_DISTRIBUTOR_CLASSNAME = "HTTP Activity Distributor";

// CIDs, or "class identifiers", are used by xpcom to uniquely
// identify a class or component. See
// http://www.mozilla.org/projects/xpcom/book/cxc/html/quicktour2.html#1005329
// for more information.
#define IMAGE_COMPRESSOR_CID                          \
{ /* 972a3d18-1fed-4bfe-9465-fd419354233e */          \
  0x972a3d18,                                         \
  0x1fed,                                             \
  0x4bfe,                                             \
  { 0x94, 0x65, 0xfd, 0x41, 0x93, 0x54, 0x23, 0x3e }  \
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
  {
    IMAGE_COMPRESSOR_CLASSNAME,
    IMAGE_COMPRESSOR_CID,
    IMAGE_COMPRESSOR_CONTRACTID,
    pagespeed::ImageCompressorConstructor,
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
