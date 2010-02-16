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
//
// JsdCallHook gets invoked for each function invocation and each
// top-level script block.

#ifndef JSD_CALL_HOOK_H_
#define JSD_CALL_HOOK_H_

#include "base/basictypes.h"
#include "base/scoped_ptr.h"

#include "jsdIDebuggerService.h"
#include "nsCOMPtr.h"

namespace activity {

class CallGraphProfile;

class JsdCallHook : public jsdICallHook {
 public:
  NS_DECL_ISUPPORTS
  NS_DECL_JSDICALLHOOK

  explicit JsdCallHook(CallGraphProfile *profile);

  // Whether or not to collect full call trees.
  void set_collect_full_call_trees(bool full) {
    collect_full_call_trees_ = full;
  }

 private:
  // Masks that we apply as we enable and disable the OnCall() filter.
  static const PRUint32 kJsdFilter;
  static const PRUint32 kScriptFilter;

  ~JsdCallHook();

  void OnEntry(jsdIStackFrame *frame, bool is_top_level);
  void OnExit(jsdIStackFrame *frame);

  // Apply or clear the filter that prevents us from being invoked at
  // every call site, depending on the value of the 'filter'
  // parameter.
  void UpdateCallFilter(jsdIStackFrame *frame, bool filter);

  int GetStackDepth(jsdIStackFrame *frame);
  bool IsFunctionNamePopulated(jsdIStackFrame *frame);

  bool IsCallFilterActive() const;

  nsCOMPtr<jsdIDebuggerService> jsd_;

  CallGraphProfile *const profile_;

  // The stack depth at the time we applied the filter. We track this
  // so we can find the matching stack depth at which to remove the
  // filter.
  int filter_depth_;

  // The number of elements in the stack that were traversed in order
  // to reach a function that should be included in the profile.
  int pending_depth_;

  // Whether or not we should apply the filter on the next call to
  // OnEntry().
  bool apply_filter_delayed_;

  // Whether or not we should collect a complete profile.
  bool collect_full_call_trees_;

  // Whether or not we've started collecting the profile.
  bool started_profiling_;

  DISALLOW_COPY_AND_ASSIGN(JsdCallHook);
};

}  // namespace activity

#endif  // JSD_CALL_HOOK_H_
