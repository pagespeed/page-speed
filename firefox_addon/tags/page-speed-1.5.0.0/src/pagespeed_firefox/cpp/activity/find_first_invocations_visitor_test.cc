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

#include "base/scoped_ptr.h"
#include "call_graph.h"
#include "call_graph_profile.h"
#include "clock.h"
#include "profile.pb.h"
#include "test_stub_function_info.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace {

const char kTestUrl1[] = "http://foo.com/index.html";
const char kTestUrl2[] = "http://bar.com/index.html";

const char kTestName1[] = "f1";
const char kTestName2[] = "f2";
const char kTestName3[] = "f3";

const char kTestSource1[] = "function f1() {}";
const char kTestSource2[] = "function f2() {}";
const char kTestSource3[] = "function f3() {}";

class FindFirstInvocationsVisitorTest : public testing::Test {
 protected:
  virtual void SetUp() {
    clock_.reset(new activity_testing::MockClock());
    profile_.reset(new activity::CallGraphProfile(clock_.get()));
    profile_->Start();
    function_info_1_.reset(new activity_testing::TestStubFunctionInfo(
                               1, kTestUrl1, kTestName1, kTestSource1));
    function_info_2_.reset(new activity_testing::TestStubFunctionInfo(
                               2, kTestUrl2, kTestName2, kTestSource2));
    function_info_3_.reset(new activity_testing::TestStubFunctionInfo(
                               3, kTestUrl1, kTestName3, kTestSource3));

    AppendTrace();
  }

  virtual void TearDown() {
    StopProfiling();
  }

  void StopProfiling() {
    if (profile_->profiling()) {
      profile_->Stop();
    }
  }

  void AppendTrace() {
    /* Append the following call tree:
          1
         / \
        1   2
       /   / \
      1   2   1

    */

    profile_->OnFunctionEntry();  // 1
    profile_->OnFunctionEntry();  // 1
    profile_->OnFunctionEntry();  // 1
    profile_->OnFunctionExit(function_info_1_.get());
    profile_->OnFunctionExit(function_info_1_.get());
    profile_->OnFunctionEntry();  // 2
    profile_->OnFunctionEntry();  // 2
    profile_->OnFunctionExit(function_info_2_.get());
    profile_->OnFunctionEntry();  // 1
    profile_->OnFunctionExit(function_info_1_.get());
    profile_->OnFunctionExit(function_info_2_.get());
    profile_->OnFunctionExit(function_info_1_.get());
  }

  scoped_ptr<activity_testing::MockClock> clock_;
  scoped_ptr<activity::CallGraphProfile> profile_;
  scoped_ptr<activity::FunctionInfoInterface> function_info_1_;
  scoped_ptr<activity::FunctionInfoInterface> function_info_2_;
  scoped_ptr<activity::FunctionInfoInterface> function_info_3_;
};

TEST_F(FindFirstInvocationsVisitorTest, BasicTraversal) {
  activity::FindFirstInvocationsVisitor visitor;
  profile_->call_graph()->Traverse(&visitor);

  const activity::FindFirstInvocationsVisitor::FirstInvocations &invocations =
      *visitor.invocations();
  ASSERT_EQ(2, invocations.size());
  ASSERT_EQ(&profile_->profile()->call_tree(0), invocations[0]);
  ASSERT_EQ(&profile_->profile()->call_tree(0).children(1), invocations[1]);

  const activity::FindFirstInvocationsVisitor::InvokedFunctionTags &tags =
      *visitor.invoked_tags();
  ASSERT_EQ(2, tags.size());
  ASSERT_EQ(1, tags.count(1));
  ASSERT_EQ(1, tags.count(2));
  ASSERT_EQ(0, tags.count(3));
}

}  // namespace
