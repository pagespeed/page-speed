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

// Author: Antonio Vicente
//
// Validate CallGraph creation, destruction and traversal methods.

#include "base/scoped_ptr.h"
#include "base/string_util.h"
#include "call_graph.h"
#include "call_graph_visitor_interface.h"
#include "call_graph_visit_filter_interface.h"
#include "clock.h"
#include "profile.pb.h"
#include "timer.h"

#include "testing/gtest/include/gtest/gtest.h"

#ifdef _WINDOWS

// Provide LONG_LONG_MAX, which is available under a different name on
// windows.
#define LONG_LONG_MAX _I64_MAX

#endif

namespace {

// The trace strings we expect the ToStringVisitor to generate.
const char kEntryTrace1[] =
    "(1)"
    "(1,2)"
    "(1,2,3)";

const char kExitTrace1[] =
    "(1,2,3)"
    "(1,2)"
    "(1)";

const char kEntryTrace2[] =
    "(1)"
    "(1,2)"
    "(1,2,2)"
    "(1,2,2,3)"
    "(1,2,2,5)"
    "(1,4)"
    "(1,6)"
    "(1,6,5)";

const char kExitTrace2[] =
    "(1,2,2,3)"
    "(1,2,2,5)"
    "(1,2,2)"
    "(1,2)"
    "(1,4)"
    "(1,6,5)"
    "(1,6)"
    "(1)";

// Generates a string representation of the traversal to test
// iteration order and contents of the CallGraph object.
class ToStringVisitor : public activity::CallGraphVisitorInterface {
 public:
  explicit ToStringVisitor(activity::CallGraphVisitFilterInterface *filter)
      : activity::CallGraphVisitorInterface(filter) {}
  ToStringVisitor() : activity::CallGraphVisitorInterface(NULL) {}
  virtual void OnEntry(const std::vector<const activity::CallTree*>& stack) {
    ASSERT_FALSE(stack.empty());
    const activity::CallTree* last = stack.back();
    ASSERT_NE(last->entry_time_usec(), -1);

    onentry_trace_ += ToString(stack);
  }

  virtual void OnExit(const std::vector<const activity::CallTree*>& stack) {
    ASSERT_FALSE(stack.empty());
    const activity::CallTree* last = stack.back();
    ASSERT_NE(last->function_tag(), -1);
    ASSERT_NE(last->exit_time_usec(), -1);

    onexit_trace_ += ToString(stack);
  }

  std::string ToString(const std::vector<const activity::CallTree*>& stack) {
    std::string str;
    str.append("(");
    for (int i = 0; i < stack.size(); i++) {
      if (i != 0) {
        str.append(",");
      }
      str.append(StringPrintf("%d", stack[i]->function_tag()));
    }
    str.append(")");
    return str;
  }

  std::string onentry_trace_;
  std::string onexit_trace_;
};

class CallGraphTest : public testing::Test {
 protected:
  virtual void SetUp() {
    profile_.reset(new activity::Profile);
    clock_.reset(new activity_testing::MockClock());
    timer_.reset(new activity::Timer(clock_.get(), clock_->GetCurrentTimeUsec()));
    graph_.reset(new activity::CallGraph(profile_.get(), timer_.get()));
    profile_->set_start_time_usec(clock_->GetCurrentTimeUsec());
  }

  virtual void TearDown() {
    graph_.reset();
    profile_.reset();
  }

  void AppendTrace1() {
    /* Append the following simple call tree:
          1
         /
        2
       /
      3
    */

    graph_->OnFunctionEntry();  // 1
    graph_->OnFunctionEntry();  // 2
    graph_->OnFunctionEntry();  // 3
    graph_->OnFunctionExit(3);  // 3
    graph_->OnFunctionExit(2);  // 2
    graph_->OnFunctionExit(1);  // 1
  }

  void AppendTrace2() {
    /* Append the following call tree:
               1
             / | \
            2  4  6
           /     /
          2     5
        /  \
       3    5
    */

    graph_->OnFunctionEntry();  // 1
    graph_->OnFunctionEntry();  // 2
    graph_->OnFunctionEntry();  // 2
    graph_->OnFunctionEntry();  // 3
    graph_->OnFunctionExit(3);  // 3
    graph_->OnFunctionEntry();  // 5
    graph_->OnFunctionExit(5);  // 5
    graph_->OnFunctionExit(2);  // 2
    graph_->OnFunctionExit(2);  // 2
    graph_->OnFunctionEntry();  // 4
    graph_->OnFunctionExit(4);  // 4
    graph_->OnFunctionEntry();  // 6
    graph_->OnFunctionEntry();  // 5
    graph_->OnFunctionExit(5);  // 5
    graph_->OnFunctionExit(6);  // 6
    graph_->OnFunctionExit(1);  // 1
  }

  void AssertTrace1(ToStringVisitor *visitor) {
    ASSERT_STREQ(kEntryTrace1, visitor->onentry_trace_.c_str());
    ASSERT_STREQ(kExitTrace1, visitor->onexit_trace_.c_str());
  }

  void AssertTrace2(ToStringVisitor *visitor) {
    ASSERT_STREQ(kEntryTrace2, visitor->onentry_trace_.c_str());
    ASSERT_STREQ(kExitTrace2, visitor->onexit_trace_.c_str());
  }

  void AssertTrace1And2(ToStringVisitor *visitor) {
    std::string entry_trace_1_and_2(kEntryTrace1);
    entry_trace_1_and_2 += kEntryTrace2;
    std::string exit_trace_1_and_2(kExitTrace1);
    exit_trace_1_and_2 += kExitTrace2;
    ASSERT_STREQ(entry_trace_1_and_2.c_str(), visitor->onentry_trace_.c_str());
    ASSERT_STREQ(exit_trace_1_and_2.c_str(), visitor->onexit_trace_.c_str());
  }

  scoped_ptr<activity_testing::MockClock> clock_;
  scoped_ptr<activity::Timer> timer_;
  scoped_ptr<activity::Profile> profile_;
  scoped_ptr<activity::CallGraph> graph_;
};

// CallGraphTest that appends both traces and records the times around
// each append operation.
class TimedCallGraphTest : public CallGraphTest {
 protected:
  virtual void SetUp() {
    CallGraphTest::SetUp();

    AppendTrace1();
    start_of_first_trace_ = profile_->call_tree(0).entry_time_usec();
    end_of_first_trace_ = profile_->call_tree(0).exit_time_usec();

    AppendTrace2();
    start_of_second_trace_ = profile_->call_tree(1).entry_time_usec();
    end_of_second_trace_ = profile_->call_tree(1).exit_time_usec();
  }

  int64 start_of_first_trace_;
  int64 end_of_first_trace_;
  int64 start_of_second_trace_;
  int64 end_of_second_trace_;
};

TEST_F(CallGraphTest, MemoryTest) {
  // verify constructing and destructing an empty object works
}

TEST_F(CallGraphTest, Traversal0Test) {
  // This test verifies that traversal works when the call graph is
  // empty
  ToStringVisitor visitor;
  graph_->Traverse(&visitor);
}

TEST_F(CallGraphTest, Traversal1Test) {
  // This test verifies that traversal works on simple graphs
  AppendTrace1();
  ToStringVisitor visitor;
  graph_->Traverse(&visitor);
  AssertTrace1(&visitor);
}

TEST_F(CallGraphTest, Traversal2Test) {
  // This test verifies that traversal works on simple graphs
  AppendTrace2();
  ToStringVisitor visitor;
  graph_->Traverse(&visitor);
  AssertTrace2(&visitor);
}

TEST_F(CallGraphTest, ForestTraversalTest) {
  // This test verifies that traversal works on graphs with multiple
  // toplevel nodes

  AppendTrace1();
  AppendTrace2();
  ToStringVisitor visitor;
  graph_->Traverse(&visitor);
  AssertTrace1And2(&visitor);
}

TEST_F(TimedCallGraphTest, TimeRangeTraversalTestFullWindow) {
  ToStringVisitor visitor(
      new activity::TimeRangeVisitFilter(
          start_of_first_trace_, end_of_second_trace_));
  graph_->Traverse(&visitor);
  AssertTrace1And2(&visitor);
}

TEST_F(TimedCallGraphTest, TimeRangeTraversalTestFirstHalf) {
  ToStringVisitor visitor(
      new activity::TimeRangeVisitFilter(
          start_of_first_trace_, end_of_first_trace_));
  graph_->Traverse(&visitor);
  AssertTrace1(&visitor);
}

TEST_F(TimedCallGraphTest, TimeRangeTraversalTestLastHalf) {
  ToStringVisitor visitor(
      new activity::TimeRangeVisitFilter(
          start_of_second_trace_, end_of_second_trace_));
  graph_->Traverse(&visitor);
  AssertTrace2(&visitor);
}

TEST_F(TimedCallGraphTest, TimeRangeTraversalBeforeTraces) {
  ToStringVisitor visitor(
      new activity::TimeRangeVisitFilter(0, start_of_first_trace_));
  graph_->Traverse(&visitor);
  ASSERT_STREQ("", visitor.onentry_trace_.c_str());
  ASSERT_STREQ("", visitor.onexit_trace_.c_str());
}

TEST_F(TimedCallGraphTest, TimeRangeTraversalTestAfterTraces) {
  ToStringVisitor visitor(
      new activity::TimeRangeVisitFilter(
          end_of_second_trace_ + 1LL, LONG_LONG_MAX));
  graph_->Traverse(&visitor);
  ASSERT_STREQ("", visitor.onentry_trace_.c_str());
  ASSERT_STREQ("", visitor.onexit_trace_.c_str());
}

TEST_F(TimedCallGraphTest, CompositeVisitFilterTest) {
  // Append one more trace. Now we'll have appended trace1, followed
  // by trace2, followed by trace1 again.
  AppendTrace1();
  const int64 end_of_third_trace = profile_->call_tree(2).exit_time_usec();

  // Construct a CompositeVisitFilter composed of two
  // TimeRangeVisitFilters. The first TimeRangeVisitFilter restricts
  // from before the first append to after the second append. The
  // second TimeRangeVisitFilter restricts from after the first append
  // to the very end. In combination, they are expected to limit the
  // visit to only the second append operation (trace 2).
  activity::CompositeVisitFilter *filter = new activity::CompositeVisitFilter(
      new activity::TimeRangeVisitFilter(
          start_of_first_trace_, end_of_second_trace_),
      new activity::TimeRangeVisitFilter(
          start_of_second_trace_, end_of_third_trace));

  ToStringVisitor visitor(filter);
  graph_->Traverse(&visitor);
  AssertTrace2(&visitor);
}

TEST_F(CallGraphTest, IncompleteTraversalTest) {
  // This test verifies that traversal works when some nodes have
  // incomplete information.

  AppendTrace1();

  graph_->OnFunctionEntry();  // 1
  graph_->OnFunctionEntry();  // 2
  graph_->OnFunctionExit(2);  // 2
  graph_->OnFunctionEntry();  // 3

  // tags and end times for nodes 1 and 3 are not known.
  ToStringVisitor visitor1;
  graph_->Traverse(&visitor1);

  ASSERT_STREQ("(1)(1,2)(1,2,3)",
               visitor1.onentry_trace_.c_str());
  ASSERT_STREQ("(1,2,3)(1,2)(1)",
               visitor1.onexit_trace_.c_str());

  graph_->OnFunctionExit(3);  // 3
  graph_->OnFunctionExit(1);  // 1

  ToStringVisitor visitor2;
  graph_->Traverse(&visitor2);

  ASSERT_STREQ("(1)(1,2)(1,2,3)(1)(1,2)(1,3)",
               visitor2.onentry_trace_.c_str());
  ASSERT_STREQ("(1,2,3)(1,2)(1)(1,2)(1,3)(1)",
               visitor2.onexit_trace_.c_str());
}

TEST_F(CallGraphTest, DeleteIncompleteTest) {
  // This test verifies that traversal works when some nodes have
  // incomplete information.

  AppendTrace1();

  graph_->OnFunctionEntry();  // 1
  graph_->OnFunctionEntry();  // 2
  graph_->OnFunctionExit(2);  // 2
  graph_->OnFunctionEntry();  // 3

  graph_.reset();
  profile_.reset();
}

}  // namespace
