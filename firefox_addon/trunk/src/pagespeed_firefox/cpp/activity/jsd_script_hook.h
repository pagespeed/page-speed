/**
 * Copyright 2008-2009 Google Inc.
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
//
// JsdScriptHook implements the JSD's jsdIScriptHook interface, and
// gets invoked when a function is loaded (parsed) or unloaded by the
// Firefox JavaScript runtime.

#ifndef JSD_SCRIPT_HOOK_H_
#define JSD_SCRIPT_HOOK_H_

#include "base/basictypes.h"

#include "jsdIDebuggerService.h"

namespace activity {

class CallGraphProfile;

// See comment at top of file for a complete description
class JsdScriptHook : public jsdIScriptHook {
 public:
  NS_DECL_ISUPPORTS
  NS_DECL_JSDISCRIPTHOOK

  explicit JsdScriptHook(CallGraphProfile *profile);

 private:
  ~JsdScriptHook();

  CallGraphProfile *const profile_;

  DISALLOW_COPY_AND_ASSIGN(JsdScriptHook);
};

}  // namespace activity

#endif  // JSD_SCRIPT_HOOK_H_
