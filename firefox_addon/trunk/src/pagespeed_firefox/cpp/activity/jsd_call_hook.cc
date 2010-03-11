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

#include "jsd_call_hook.h"

#include "call_graph_profile.h"
#include "check.h"
#include "jsd_function_info.h"

#include "nsCOMPtr.h"
#include "nsServiceManagerUtils.h"

NS_IMPL_ISUPPORTS1(activity::JsdCallHook, jsdICallHook)

namespace {
const char* kJsdContractStr = "@mozilla.org/js/jsd/debugger-service;1";
}

namespace activity {

JsdCallHook::JsdCallHook(CallGraphProfile *profile)
    : jsd_(do_GetService(kJsdContractStr)),
      profile_(profile),
      filter_depth_(-1),
      collect_full_call_trees_(false),
      started_profiling_(false) {
}

JsdCallHook::~JsdCallHook() {}

NS_IMETHODIMP JsdCallHook::OnCall(jsdIStackFrame *frame, PRUint32 type) {
  if (jsd_ == NULL) {
    // If we were unable to get a handle to the JSD, bail.
    return NS_ERROR_FAILURE;
  }

  if (!started_profiling_) {
    if (!CanStartProfiling(frame, type)) {
      // The call stack is not in a state that allows us to start
      // profiling, so don't record this call.
      return NS_OK;
    }

    // We're starting to profile, so reset our state.
    filter_depth_ = -1;
    started_profiling_ = true;
  }

  switch (type) {
    case jsdICallHook::TYPE_FUNCTION_CALL:
    case jsdICallHook::TYPE_TOPLEVEL_START: {
      OnEntry(frame);
      break;
    }

    case jsdICallHook::TYPE_FUNCTION_RETURN:
    case jsdICallHook::TYPE_TOPLEVEL_END: {
      OnExit(frame);
      break;
    }

    default: {
      // other events ignored.
      break;
    }
  }

  return NS_OK;
}

void JsdCallHook::OnEntry(jsdIStackFrame *frame) {
  nsCOMPtr<jsdIScript> script;
  nsresult rv = frame->GetScript(getter_AddRefs(script));
  if (NS_FAILED(rv)) {
    GCHECK(false);
    return;
  }

  JsdFunctionInfo function_info(script);

  if (collect_full_call_trees_) {
    // If we're collecting full call trees, just record this as a
    // normal function entry point.
    profile_->OnFunctionEntry(&function_info);
    return;
  }

  if (IsCallFilterActive()) {
    // If already active, we don't need to collect any additional
    // information, so bail.
    return;
  }

  if (!profile_->ShouldIncludeInProfile(function_info.GetFileName())) {
    return;
  }

  profile_->OnFunctionEntry(&function_info);

  // We found a function that should be included in the profile, so
  // apply the call filter.
  UpdateCallFilter(frame, true);
}

void JsdCallHook::OnExit(jsdIStackFrame *frame) {
  nsCOMPtr<jsdIScript> script;
  nsresult rv = frame->GetScript(getter_AddRefs(script));
  if (NS_FAILED(rv)) {
    GCHECK(false);
    return;
  }

  JsdFunctionInfo function_info(script);
  if (collect_full_call_trees_) {
    // If we're collecting full call trees, just record this as a
    // normal function exit point.
    profile_->OnFunctionExit(&function_info);
    return;
  }

  if (!profile_->ShouldIncludeInProfile(function_info.GetFileName())) {
    return;
  }

  GCHECK(IsCallFilterActive());
  if (filter_depth_ == GetStackDepth(frame)) {
    // We're at the function return point that matches the point where we
    // applied the filter, so we should un-apply the filter here.
    profile_->OnFunctionExit(&function_info);
    UpdateCallFilter(frame, false);
  }
}

bool JsdCallHook::CanStartProfiling(jsdIStackFrame *frame, PRUint32 type) const {
  // We have to catch the case where we start profiling in the
  // middle of a call stack. We don't want to start recording
  // function calls until we begin our first complete call graph.
  if (type != jsdICallHook::TYPE_FUNCTION_CALL &&
      type != jsdICallHook::TYPE_TOPLEVEL_START) {
    // Only start profiling on a function call/toplevel start (never
    // start on a function return).
    return false;
  }

  if (GetStackDepth(frame) != 1) {
    // Only start profiling if we're at the bottom of the call stack.
    return false;
  }

  return true;
}

bool JsdCallHook::IsCallFilterActive() const {
  return filter_depth_ >= 0;
}

void JsdCallHook::UpdateCallFilter(jsdIStackFrame *frame, bool filter) {
  nsresult rv = NS_OK;
  nsCOMPtr<jsdIScript> script;
  rv = frame->GetScript(getter_AddRefs(script));
  if (NS_FAILED(rv)) {
    GCHECK(false);
    return;
  }

  PRUint32 jsd_flags = 0;
  rv = jsd_->GetFlags(&jsd_flags);
  if (NS_FAILED(rv)) {
    GCHECK(false);
    return;
  }
  PRUint32 script_flags = 0;
  rv = script->GetFlags(&script_flags);
  if (NS_FAILED(rv)) {
    GCHECK(false);
    return;
  }
  // Ideally, we would have the Firefox JSD call us at every call site
  // in order to construct a complete JavaScript call
  // graph. Unfortunately, the overhead of doing so is very high, and
  // it introduces a significant "observer effect". This is mostly due
  // to the fact that the JSD allocates space for and populates a new
  // copy of the entire call stack at each call site. We don't
  // actually care about the call stack, so we'd like to be able to
  // disable this behavior. Until it's possible to do so, we leverage
  // the Firefox JSD's debug filter, which allows us to disable
  // construction of the call stack for all but the function at the
  // bottom of the stack. Ideally we'd like to apply the filter to the
  // stack frame, not the function, but being able to filter on the
  // function is better than nothing. This lets us build a call graph
  // that captures the top-level entry and exit times, which allows us
  // to render the JavaScript execution on the timeline without
  // introducing an observer effect.
  if (filter) {
    // Enable the filter for the JSD and the script, and record the
    // current stack depth.
    jsd_flags |= kJsdFilter;
    script_flags |= kScriptFilter;
    filter_depth_ = GetStackDepth(frame);
  } else {
    // Disable the filter for the JSD and the script, and clear the
    // stack depth.
    jsd_flags &= ~kJsdFilter;
    script_flags &= ~kScriptFilter;
    filter_depth_ = -1;
  }
  nsresult jsd_rv = jsd_->SetFlags(jsd_flags);
  nsresult script_rv = script->SetFlags(script_flags);
  if (NS_FAILED(jsd_rv)) {
    GCHECK(false);
    return;
  }
  if (NS_FAILED(script_rv)) {
    GCHECK(false);
    return;
  }
}

int JsdCallHook::GetStackDepth(jsdIStackFrame *frame) const {
  nsresult rv = NS_OK;
  int depth = 0;
  for (nsCOMPtr<jsdIStackFrame> current = frame, last = NULL;
       current != NULL;
       last = current, rv = last->GetCallingFrame(getter_AddRefs(current))) {
    if (NS_FAILED(rv)) {
      GCHECK(false);
      return -1;
    }
    ++depth;
  }
  return depth;
}

const PRUint32 JsdCallHook::kJsdFilter = jsdIDebuggerService::DEBUG_WHEN_SET;
const PRUint32 JsdCallHook::kScriptFilter = jsdIScript::FLAG_DEBUG;

}  // namespace activity
