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
// FindFirstInvocationsVisitor traverses the call graph to find the
// first invocation of each function in the call
// graph.

#ifndef FIND_FIRST_INVOCATIONS_VISITOR_H_
#define FIND_FIRST_INVOCATIONS_VISITOR_H_

#include <stdint.h>

#include <vector>

#include "base/basictypes.h"
#include "call_graph_visitor_interface.h"
#include "portable_hash_set.h"

namespace activity {

class CallTree;

class FindFirstInvocationsVisitor : public CallGraphVisitorInterface {
 public:
  typedef portable_hash_set<int32_t> InvokedFunctionTags;
  typedef std::vector<const CallTree*> FirstInvocations;

  FindFirstInvocationsVisitor();
  virtual ~FindFirstInvocationsVisitor();

  const FirstInvocations *invocations() const { return &first_invocations_; }

  const InvokedFunctionTags *invoked_tags() const {
    return &function_tags_encountered_;
  }

  virtual void OnEntry(const std::vector<const CallTree*>& stack);
  virtual void OnExit(const std::vector<const CallTree*>& stack);

 private:
  FirstInvocations first_invocations_;
  InvokedFunctionTags function_tags_encountered_;

  DISALLOW_COPY_AND_ASSIGN(FindFirstInvocationsVisitor);
};

}  // namespace activity

#endif  // FIND_FIRST_INVOCATIONS_VISITOR_H_
