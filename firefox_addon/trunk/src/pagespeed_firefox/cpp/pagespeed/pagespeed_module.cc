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

#include "mozilla/ModuleUtils.h"
#include "nsIClassInfoImpl.h"

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

// The following line defines a kNS_SAMPLE_CID CID variable.
NS_DEFINE_NAMED_CID(PAGE_SPEED_RULES_CID);

// Build a table of ClassIDs (CIDs) which are implemented by this
// module. CIDs should be completely unique UUIDs.  each entry has the
// form { CID, service, factoryproc, constructorproc } where
// factoryproc is usually NULL.
static const mozilla::Module::CIDEntry kPageSpeedCIDs[] = {
    { &kPAGE_SPEED_RULES_CID,
      false,
      NULL,
      pagespeed::PageSpeedRulesConstructor },
    { NULL }
};

// Build a table which maps contract IDs to CIDs.  A contract is a
// string which identifies a particular set of functionality. In some
// cases an extension component may override the contract ID of a
// builtin gecko component to modify or extend functionality.
static const mozilla::Module::ContractIDEntry kPageSpeedContracts[] = {
    { PAGE_SPEED_RULES_CONTRACTID, &kPAGE_SPEED_RULES_CID },
    { NULL }
};

static const mozilla::Module kPageSpeedModule = {
    mozilla::Module::kVersion,
    kPageSpeedCIDs,
    kPageSpeedContracts,
    NULL  // we don't need to register for any categories
};

// The following line implements the one-and-only "NSModule" symbol
// exported from this shared library.
NSMODULE_DEFN(PageSpeedModule) = &kPageSpeedModule;

// The following line implements the one-and-only "NSGetModule" symbol
// for compatibility with mozilla 1.9.2. You should only use this
// if you need a binary which is backwards-compatible and if you use
// interfaces carefully across multiple versions.
NS_IMPL_MOZILLA192_NSGETMODULE(&kPageSpeedModule)

}  // namespace
