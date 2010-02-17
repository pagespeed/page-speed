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
// JsdScriptHook implementation

#include "jsd_script_hook.h"

#include "call_graph_profile.h"
#include "jsd_function_info.h"

NS_IMPL_ISUPPORTS1(activity::JsdScriptHook, jsdIScriptHook)

namespace activity {

JsdScriptHook::JsdScriptHook(CallGraphProfile *profile)
    : profile_(profile),
      collect_full_call_trees_(false) {
}

JsdScriptHook::~JsdScriptHook() {}

NS_IMETHODIMP JsdScriptHook::OnScriptCreated(jsdIScript *script) {
  JsdFunctionInfo function_info(script);

  if (collect_full_call_trees_ ||
      profile_->ShouldIncludeInProfile(function_info.GetFileName())) {
    profile_->OnFunctionInstantiated(&function_info);
  }
  return NS_OK;
}

NS_IMETHODIMP JsdScriptHook::OnScriptDestroyed(jsdIScript *script) {
  return NS_OK;
}

}  // namespace activity
