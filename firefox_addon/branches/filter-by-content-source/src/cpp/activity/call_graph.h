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

// CallGraph is a data structure that holds information about js
// functions executed while the extension is active.  Clients may
// access this information by means of CallGraph::VisitorInterface.
// CallGraph is not thread-safe. If you need to access a CallGraph
// instance from multiple threads, create a read-only snapshot of the
// CallGraph using CreateSnapshot.

#ifndef CALL_GRAPH_H_
#define CALL_GRAPH_H_

#include <stdint.h>

#include <vector>

#include "base/macros.h"

namespace activity {

class CallTree;
class CallGraphVisitorInterface;
class Profile;
class Timer;

// See comment at top of file for a complete description
class CallGraph {
 public:
  typedef std::vector<const CallTree*> CallForest;

  CallGraph(Profile *profile, Timer *timer);

  // These methods are used by FunctionTraceHook to populate the data
  // structure.  OnFunctionEntry adds a new, partially populated node
  // to the working set.
  // OnFunctionExit populates the remaining fields, adds toplevel
  // nodes to the root list and removes the last node from the working
  // set, which was created when we entered the corresponding
  // iteration of function.
  //
  // @INVARIANT(# calls to OnFunctionEntry >= # calls to OnFunctionExit)
  void OnFunctionEntry();
  void OnFunctionExit(int32_t tag);

  // Do dfs(execution order) traversal of the data structure.
  // Traversal is limited to nodes rooted in a member of roots_,
  // e.i. the node and none of its parents is in working_set_.
  // NOTE: calls to OnFunctionExit() may add nodes to roots_, which
  // invalidates iterators.  Never call OnFunctionExit() inside
  // Traverse().
  void Traverse(CallGraphVisitorInterface* visitor) const;

  // Is the last CallTree only partially constructed?
  bool IsPartiallyConstructed() const;

  // Create a read-only thread-safe view of this CallGraph. The
  // returned object is backed by the same data store as this object,
  // so the caller must ensure that the read-only view is destroyed
  // before this object is destroyed. Ownership of the returned object
  // is transferred to the caller.
  CallGraph *CreateSnapshot() const;

  const CallForest *call_forest() const { return &call_trees_; }

 private:
  typedef std::vector<CallTree*> CallStack;

  // List of toplevel calls into the js program traced by this
  // CallGraph object.  Toplevel nodes are added to the end of the
  // list OnFunctionExit.
  CallForest call_trees_;

  // working set that corresponds to the currently executing js stack.
  CallStack working_set_;

  Profile *const profile_;
  Timer *const timer_;

  DISALLOW_COPY_AND_ASSIGN(CallGraph);
};

}  // namespace activity

#endif  // CALL_GRAPH_H_
