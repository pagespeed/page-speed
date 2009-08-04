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
}  // namespace

namespace activity {

JsdCallHook::JsdCallHook(CallGraphProfile *profile)
    : profile_(profile),
      filter_depth_(-1),
      apply_filter_pending_(false),
      collect_full_call_trees_(false),
      started_profiling_(false) {
}

JsdCallHook::~JsdCallHook() {}

NS_IMETHODIMP JsdCallHook::OnCall(jsdIStackFrame *frame, PRUint32 type) {
  if (!started_profiling_) {
    // We have to catch the case where we start profiling in the
    // middle of a call stack. We don't want to start recording
    // function calls until we begin our first complete call graph.
    if (type != jsdICallHook::TYPE_FUNCTION_CALL &&
        type != jsdICallHook::TYPE_TOPLEVEL_START) {
      // Only start profiling on a function call/toplevel start (never
      // start on a function return).
      return NS_OK;
    }

    if (GetStackDepth(frame) != 1) {
      // Only start profiling if we're at the bottom of the call stack.
      return NS_OK;
    }
    started_profiling_ = true;
  }

  switch (type) {
    case jsdICallHook::TYPE_FUNCTION_CALL:
    case jsdICallHook::TYPE_TOPLEVEL_START: {
      const bool is_top_level = (type == jsdICallHook::TYPE_TOPLEVEL_START);
      OnEntry(frame, is_top_level);
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

void JsdCallHook::OnEntry(jsdIStackFrame *frame, bool is_top_level) {
  if (collect_full_call_trees_) {
    // If we're collecting full call trees, just record this as a
    // normal function entry point.
    profile_->OnFunctionEntry();
    return;
  }

  if (apply_filter_pending_) {
    // There is a pending request to apply the filter that prevents us
    // from being called at every js call point. Now that we've
    // re-entered the OnEntry() callback, we need to apply the
    // filter. See the comments below for details on why this pending
    // operation is necessary.
    GCHECK(!IsCallFilterActive());
    UpdateCallFilter(frame, true);
    apply_filter_pending_ = false;
    return;
  }

  if (!IsCallFilterActive()) {
    // The filter is not currently applied. This means we're just
    // beginning a new call stack. We need to record this function
    // entry point and apply the filter.
    profile_->OnFunctionEntry();
    if (is_top_level || IsFunctionNamePopulated(frame)) {
      // If this is a top-level script, or the function name is
      // available, then we can enable the filter on this script.
      UpdateCallFilter(frame, true);
    } else {
      // The Firefox JSD has a funny characteristic: when OnCall() is
      // invoked with TYPE_FUNCTION_CALL, the jsdIScript at the top of
      // the call stack might be for the previous stack frame (not for
      // the function actually being called). In cases where the stack
      // is just being constructed, this means that the top-level
      // frame will be for a dummy function with no function name. We
      // can't filter on such a function, because that function will
      // never appear in the TYPE_FUNCTION_RETURN call path. Instead,
      // we set a flag that indicates the next time OnCall() gets
      // invoked with TYPE_FUNCTION_CALL, we should apply the filter on
      // that function.
      apply_filter_pending_ = true;
    }
  }
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

  if (apply_filter_pending_) {
    // We had a filter pending, but OnEntry() wasn't invoked
    // again. This means that a function was entered and immediately
    // exited (e.g. a setTimeout() callback that doesn't call any
    // functions). Record the function exit and clear the pending
    // filter operation.
    GCHECK(!IsCallFilterActive());
    profile_->OnFunctionExit(&function_info);
    apply_filter_pending_ = false;
    return;
  }

  if (filter_depth_ == GetStackDepth(frame)) {
    // We're at the function return point that matches the point where we
    // applied the filter, so we should un-apply the filter here.
    profile_->OnFunctionExit(&function_info);
    UpdateCallFilter(frame, false);
  }
}

bool JsdCallHook::IsCallFilterActive() const {
  return filter_depth_ >= 0;
}

bool JsdCallHook::IsFunctionNamePopulated(jsdIStackFrame *frame) {
  nsCOMPtr<jsdIScript> script;
  nsresult rv = frame->GetScript(getter_AddRefs(script));
  if (NS_FAILED(rv)) {
    GCHECK(false);
    return false;
  }

  JsdFunctionInfo function_info(script);
  return function_info.GetFunctionName() == NULL ||
      function_info.GetFunctionName()[0] != '\0';
}

void JsdCallHook::UpdateCallFilter(jsdIStackFrame *frame, bool filter) {
  nsresult rv = NS_OK;
  nsCOMPtr<jsdIDebuggerService> jsd = do_GetService(kJsdContractStr, &rv);
  if (NS_FAILED(rv)) {
    GCHECK(false);
    return;
  }

  nsCOMPtr<jsdIScript> script;
  rv = frame->GetScript(getter_AddRefs(script));
  if (NS_FAILED(rv)) {
    GCHECK(false);
    return;
  }

  PRUint32 jsd_flags = 0;
  rv = jsd->GetFlags(&jsd_flags);
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
  nsresult jsd_rv = jsd->SetFlags(jsd_flags);
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

int JsdCallHook::GetStackDepth(jsdIStackFrame *frame) {
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
