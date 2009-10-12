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

#include "find_first_invocations_visitor.h"

#include "profile.pb.h"

namespace activity {

FindFirstInvocationsVisitor::FindFirstInvocationsVisitor()
    : CallGraphVisitorInterface(NULL) {
}

FindFirstInvocationsVisitor::~FindFirstInvocationsVisitor() {}

void FindFirstInvocationsVisitor::OnEntry(
    const std::vector<const CallTree*>& stack) {
  const CallTree &tree = *stack.back();
  const int32_t function_tag = tree.function_tag();
  if (function_tags_encountered_.find(function_tag) ==
      function_tags_encountered_.end()) {
    function_tags_encountered_.insert(function_tag);
    first_invocations_.push_back(&tree);
  }
}

void FindFirstInvocationsVisitor::OnExit(
    const std::vector<const CallTree*>& stack) {
}

}  // namespace activity
