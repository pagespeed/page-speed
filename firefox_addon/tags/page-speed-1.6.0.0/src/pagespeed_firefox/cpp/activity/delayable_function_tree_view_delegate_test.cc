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

#include "delayable_function_tree_view_delegate.h"

#include "base/scoped_ptr.h"
#include "call_graph.h"
#include "call_graph_profile.h"
#include "clock.h"
#include "find_first_invocations_visitor.h"
#include "profile.pb.h"
#include "test_stub_function_info.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace {

const char kTestUrl1[] = "http://foo.com/index.html";
const char kTestUrl2[] = "http://bar.com/index.html";

const char kTestName1[] = "f1";
const char kTestName2[] = "f2";

// In Firefox, a function with no name is a top-level script block. We
// want to make sure that we do not include top-level script blocks in
// the list of delayable functions.
const char kTestName3[] = "";

const char kTestSource1[] = "function f1() {}";
const char kTestSource2[] = "function f2() {}";
const char kTestSource3[] = "while (true) {}";

class DelayableFunctionTreeViewDelegateTest : public testing::Test {
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
    delegate_.reset(
        new activity::DelayableFunctionTreeViewDelegate(*profile_.get()));

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
          1         3
         / \       / \
        1   2     1   2
       /   / \
      1   2   1

    */

    // Only explicitly instantiate 2 of the 3 functions, in order to
    // verify that the delegate correctly filters out functions that
    // do not have instantiation times (function 3).
    profile_->OnFunctionInstantiated(function_info_1_.get());
    clock_->current_time_usec_ += 1000;
    profile_->OnFunctionInstantiated(function_info_2_.get());
    clock_->current_time_usec_ += 1000;
    profile_->OnFunctionInstantiated(function_info_3_.get());

    profile_->OnFunctionEntry();  // 1
    profile_->OnFunctionEntry();  // 1
    profile_->OnFunctionEntry();  // 1
    profile_->OnFunctionExit(function_info_1_.get());
    profile_->OnFunctionExit(function_info_1_.get());
    clock_->current_time_usec_ += 10000000;
    profile_->OnFunctionEntry();  // 2
    profile_->OnFunctionEntry();  // 2
    profile_->OnFunctionExit(function_info_2_.get());
    profile_->OnFunctionEntry();  // 1
    profile_->OnFunctionExit(function_info_1_.get());
    profile_->OnFunctionExit(function_info_2_.get());
    profile_->OnFunctionExit(function_info_1_.get());
    profile_->OnFunctionEntry();  // 3
    profile_->OnFunctionEntry();  // 1
    profile_->OnFunctionExit(function_info_1_.get());
    profile_->OnFunctionEntry();  // 2
    profile_->OnFunctionExit(function_info_2_.get());
    profile_->OnFunctionExit(function_info_3_.get());
  }

  void AssertCellText(
      const char *expected,
      int32 row_index,
      activity::DelayableFunctionTreeViewDelegate::ColumnId column) {
    std::string out;
    ASSERT_TRUE(delegate_->GetCellText(row_index, column, &out));
    EXPECT_STREQ(expected, out.c_str());
  }

  scoped_ptr<activity_testing::MockClock> clock_;
  scoped_ptr<activity::CallGraphProfile> profile_;
  scoped_ptr<activity::FunctionInfoInterface> function_info_1_;
  scoped_ptr<activity::FunctionInfoInterface> function_info_2_;
  scoped_ptr<activity::FunctionInfoInterface> function_info_3_;
  scoped_ptr<activity::DelayableFunctionTreeViewDelegate> delegate_;
};

TEST_F(DelayableFunctionTreeViewDelegateTest, NoRowsWhenNotInitialized) {
  ASSERT_EQ(0, delegate_->GetRowCount());

  std::string out;
  ASSERT_FALSE(
      delegate_->GetCellText(
          0, activity::DelayableFunctionTreeViewDelegate::FILE_NAME, &out));
}

TEST_F(DelayableFunctionTreeViewDelegateTest, InvalidArguments) {
  activity::FindFirstInvocationsVisitor visitor;
  profile_->call_graph()->Traverse(&visitor);
  delegate_->Initialize(visitor);

  std::string out;

  // First verify that passing valid arguments succeeds.
  ASSERT_TRUE(
      delegate_->GetCellText(
          0, activity::DelayableFunctionTreeViewDelegate::FILE_NAME, &out));

  // Verify that passing a NULL out param fails.
  EXPECT_FALSE(
      delegate_->GetCellText(
          0, activity::DelayableFunctionTreeViewDelegate::FILE_NAME, NULL));

  // Verify that passing an invalid row index fails.
  EXPECT_FALSE(
      delegate_->GetCellText(
          -1, activity::DelayableFunctionTreeViewDelegate::FILE_NAME, &out));
  EXPECT_FALSE(
      delegate_->GetCellText(
          2, activity::DelayableFunctionTreeViewDelegate::FILE_NAME, &out));

  // Verify that passing an invalid column index fails.
  EXPECT_FALSE(delegate_->GetCellText(0, -1, &out));
  EXPECT_FALSE(
      delegate_->GetCellText(
          0,
          activity::DelayableFunctionTreeViewDelegate::LAST_COLUMN_ID + 1, &out));
}

TEST_F(DelayableFunctionTreeViewDelegateTest, RowContents) {
  activity::FindFirstInvocationsVisitor visitor;
  profile_->call_graph()->Traverse(&visitor);
  delegate_->Initialize(visitor);

  ASSERT_EQ(2, delegate_->GetRowCount());

  // Verify contents of first row.
  AssertCellText(
      "10 seconds",
      0, activity::DelayableFunctionTreeViewDelegate::DELAY);
  AssertCellText(
      "1 ms",
      0,
      activity::DelayableFunctionTreeViewDelegate::INSTANTIATION_TIME);
  AssertCellText(
      "10 seconds",
      0, activity::DelayableFunctionTreeViewDelegate::FIRST_CALL);
  AssertCellText(
      kTestName2,
      0, activity::DelayableFunctionTreeViewDelegate::FUNCTION_NAME);
  AssertCellText(
      kTestSource2,
      0, activity::DelayableFunctionTreeViewDelegate::FUNCTION_SOURCE);
  AssertCellText(
      kTestUrl2,
      0, activity::DelayableFunctionTreeViewDelegate::FILE_NAME);

  // Verify contents of second row.
  AssertCellText(
      "2 ms",
      1, activity::DelayableFunctionTreeViewDelegate::DELAY);
  AssertCellText(
      "0 ms",
      1,
      activity::DelayableFunctionTreeViewDelegate::INSTANTIATION_TIME);
  AssertCellText(
      "2 ms",
      1, activity::DelayableFunctionTreeViewDelegate::FIRST_CALL);
  AssertCellText(
      kTestName1,
      1, activity::DelayableFunctionTreeViewDelegate::FUNCTION_NAME);
  AssertCellText(
      kTestSource1,
      1, activity::DelayableFunctionTreeViewDelegate::FUNCTION_SOURCE);
  AssertCellText(
      kTestUrl1,
      1, activity::DelayableFunctionTreeViewDelegate::FILE_NAME);
}

}  // namespace
