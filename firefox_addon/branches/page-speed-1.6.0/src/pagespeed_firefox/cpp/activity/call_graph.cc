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

// CallGraph implementation.  See call_graph.h for details.

#include "call_graph.h"

#include "call_graph_visitor_interface.h"
#include "profile.pb.h"
#include "check.h"
#include "timer.h"

namespace activity {

CallGraph::CallGraph(Profile *profile, Timer *timer)
    : profile_(profile),
      timer_(timer) {
}

CallGraph *CallGraph::CreateSnapshot() const {
  CallGraph *graph = new CallGraph(profile_, timer_);
  graph->call_trees_ = call_trees_;
  graph->working_set_ = working_set_;
  return graph;
}

void CallGraph::OnFunctionEntry() {
  CallTree* node = NULL;
  if (working_set_.empty()) {
    node = profile_->add_call_tree();
  } else {
    node = working_set_.back()->add_children();
  }

  node->set_entry_time_usec(timer_->GetElapsedTimeUsec());
  working_set_.push_back(node);
}

void CallGraph::OnFunctionExit(int32 tag) {
  GCHECK(!working_set_.empty());

  CallTree* node = working_set_.back();
  node->set_function_tag(tag);
  node->set_exit_time_usec(timer_->GetElapsedTimeUsec());
  working_set_.pop_back();

  if (working_set_.empty()) {
    call_trees_.push_back(node);
  }
}

bool CallGraph::IsPartiallyConstructed() const {
  return !working_set_.empty();
}

void CallGraph::Traverse(CallGraphVisitorInterface* visitor) const {
  for (CallForest::const_iterator it = call_trees_.begin(),
           end = call_trees_.end();
       it != end;
       ++it) {
    std::vector<const CallTree*> stack;
    const CallTree* call_tree = *it;
    GCHECK(call_tree != NULL);
    CallGraphVisitorInterface::Traverse(visitor, *call_tree, &stack);
  }
}

}  // namespace activity
