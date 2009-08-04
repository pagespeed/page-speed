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

// Interface used by CallGraph::Traverse to communicate with clients
// that want access to data gathered in this data structure.

#ifndef CALL_GRAPH_VISITOR_INTERFACE_H_
#define CALL_GRAPH_VISITOR_INTERFACE_H_

#include <vector>

#include "base/macros.h"
#include "base/scoped_ptr.h"

namespace activity {

class CallGraphVisitFilterInterface;
class CallTree;

// See comment at top of file for a complete description
class CallGraphVisitorInterface {
 public:
  /**
   * Visit the nodes, using the optional specified filter to prune
   * subtrees. If no filtering should be performed, pass NULL. On
   * construction, ownership of the CallGraphVisitFilterInterface
   * instance is transferred to this object, which will delete it in
   * its destructor.
   */
  explicit CallGraphVisitorInterface(CallGraphVisitFilterInterface *filter);
  virtual ~CallGraphVisitorInterface();

  virtual void OnEntry(const std::vector<const CallTree*>& stack) = 0;
  virtual void OnExit(const std::vector<const CallTree*>& stack) = 0;
  static void Traverse(CallGraphVisitorInterface* visitor,
                       const CallTree& tree,
                       std::vector<const CallTree*>* parent_stack);
 private:
  scoped_ptr<CallGraphVisitFilterInterface> visit_filter_;

  DISALLOW_COPY_AND_ASSIGN(CallGraphVisitorInterface);
};

}  // namespace activity

#endif  // CALL_GRAPH_VISITOR_INTERFACE_H_
