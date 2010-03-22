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

// CallGraphVisitorInterface implementation

#include "call_graph_visitor_interface.h"

#include "base/logging.h"
#include "call_graph_visit_filter_interface.h"
#include "profile.pb.h"

namespace activity {

CallGraphVisitorInterface::CallGraphVisitorInterface(
    CallGraphVisitFilterInterface *filter)
    : visit_filter_((filter != NULL) ? filter : new AlwaysVisitFilter()) {
  if (visit_filter_ == NULL) {
    LOG(DFATAL) << "Null visit_filter_";
  }
}

CallGraphVisitorInterface::~CallGraphVisitorInterface() {}

void CallGraphVisitorInterface::Traverse(
    CallGraphVisitorInterface* visitor,
    const CallTree& tree,
    std::vector<const CallTree*>* parent_stack) {
  if (visitor == NULL || parent_stack == NULL) {
    LOG(DFATAL) << "Bad params to Traverse: " << visitor << "," << parent_stack;
    return;
  }

  if (!visitor->visit_filter_->ShouldTraverse(tree, *parent_stack)) {
    return;
  }

  // Temporarily modify the parent stack to make it our own.  A
  // cleaner alternative would be to represent the stack as a linked
  // list, and push new stack frames to the front.
  parent_stack->push_back(&tree);
  visitor->OnEntry(*parent_stack);

  for (int i = 0, size = tree.children_size(); i < size; i++) {
    const CallTree& child = tree.children(i);
    Traverse(visitor, child, parent_stack);
  }

  visitor->OnExit(*parent_stack);
  parent_stack->pop_back();
}

}  // namespace activity
