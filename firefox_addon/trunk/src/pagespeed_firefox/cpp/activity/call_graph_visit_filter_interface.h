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
// Interface used by CallGraphVisitorInterface to determine if a
// CallGraphVisitorInterface should traverse the given node and its
// children.

#ifndef CALL_GRAPH_VISIT_FILTER_INTERFACE_H_
#define CALL_GRAPH_VISIT_FILTER_INTERFACE_H_

#include <stdint.h>

#include <vector>

#include "base/basictypes.h"
#include "base/scoped_ptr.h"

namespace activity {

class CallTree;

/**
 * CallGraphVisitFilterInterface defines a policy on whether or not to
 * visit each node.
 */
class CallGraphVisitFilterInterface {
 public:
  CallGraphVisitFilterInterface();
  virtual ~CallGraphVisitFilterInterface();

  /**
   * Should the containing CallGraphVisitorInterface traverse the
   * given node and its children?
   */
  virtual bool ShouldTraverse(
      const CallTree &tree,
      const std::vector<const CallTree*>& parent_stack) = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(CallGraphVisitFilterInterface);
};

/**
 * Filter that visits all nodes. This is the default filter if
 * none is specified in the CallGraphVisitorInterface constructor.
 */
class AlwaysVisitFilter : public CallGraphVisitFilterInterface {
 public:
  AlwaysVisitFilter();
  virtual ~AlwaysVisitFilter();
  virtual bool ShouldTraverse(
      const CallTree &tree, const std::vector<const CallTree*>& parent_stack);

 private:
  DISALLOW_COPY_AND_ASSIGN(AlwaysVisitFilter);
};

/**
 * A CompositeVisitFilter is composed of multiple
 * CallGraphVisitFilterInterface instances. If any one of the
 * instances returns false from ShouldTraverse, the
 * CompositeVisitFilter will return false to ShouldTraverse as well.
 */
class CompositeVisitFilter : public CallGraphVisitFilterInterface {
 public:
  /**
   * Construct a CompositeVisitFilter composed of the two filters
   * specified. The CompositeVisitFilter takes ownership of the passed
   * in filters and will delete them in its destructor.
   */
  CompositeVisitFilter(
      CallGraphVisitFilterInterface *first,
      CallGraphVisitFilterInterface *second);
  virtual ~CompositeVisitFilter();

  virtual bool ShouldTraverse(
      const CallTree &tree, const std::vector<const CallTree*>& parent_stack);

 private:
  scoped_ptr<CallGraphVisitFilterInterface> first_;
  scoped_ptr<CallGraphVisitFilterInterface> second_;

  DISALLOW_COPY_AND_ASSIGN(CompositeVisitFilter);
};

/**
 * A TimeRangeVisitFilter only visits nodes whose execution at
 * least partially overlaps the specified time window, inclusive.
 */
class TimeRangeVisitFilter : public CallGraphVisitFilterInterface {
 public:
  TimeRangeVisitFilter(
      int64_t start_time_usec,
      int64_t end_time_usec);
  virtual ~TimeRangeVisitFilter();
  virtual bool ShouldTraverse(
      const CallTree &tree, const std::vector<const CallTree*>& parent_stack);

 private:
  int64_t start_time_usec_;
  int64_t end_time_usec_;

  DISALLOW_COPY_AND_ASSIGN(TimeRangeVisitFilter);
};

}  // namespace activity

#endif  // CALL_GRAPH_VISIT_FILTER_INTERFACE_H_
