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
// Verify CallGraphProfile behavior.

#include "base/scoped_ptr.h"
#include "call_graph.h"
#include "call_graph_profile.h"
#include "clock.h"
#include "profile.pb.h"
#include "test_stub_function_info.h"

#include "testing/gtest/include/gtest/gtest.h"

class CallGraphProfileTest : public testing::Test {
 protected:
  virtual void SetUp() {
    clock_.reset(new activity_testing::MockClock());
    profile_.reset(new activity::CallGraphProfile(clock_.get()));
  }

  virtual void TearDown() {
    profile_.reset();
  }

  void OnFunctionEntry() {
    profile_->OnFunctionEntry();
  }

  void OnFunctionExit(int32 tag) {
    activity_testing::TestStubFunctionInfo function_info(tag);
    profile_->OnFunctionExit(&function_info);
  }

  const activity::Profile *GetProfile() { return profile_->profile(); }

  scoped_ptr<activity_testing::MockClock> clock_;
  scoped_ptr<activity::CallGraphProfile> profile_;
};

namespace {

TEST_F(CallGraphProfileTest, OnFunctionEntryExitFailsWhenNotProfiling) {
  ASSERT_DEATH(OnFunctionEntry(), "Check failed: profiling\\(\\)");
  ASSERT_DEATH(OnFunctionExit(-1), "Check failed: profiling\\(\\)");
}

TEST_F(CallGraphProfileTest, OnFunctionExitFailsWithNoWorkingSet) {
  profile_->Start();
  ASSERT_DEATH(OnFunctionExit(-1), "Check failed: !working_set_.empty\\(\\)");
}

TEST_F(CallGraphProfileTest, SerializeFailsWhenProfiling) {
  profile_->Start();
  ASSERT_DEATH(profile_->SerializeToFileDescriptor(-1),
               "Check failed: !profiling\\(\\)");
}

TEST_F(CallGraphProfileTest, LastParitalCallTreeGetsRemoved) {
  profile_->Start();

  OnFunctionEntry();
  OnFunctionEntry();
  OnFunctionExit(2);
  OnFunctionExit(1);

  // Verify that we have exactly one call tree.
  ASSERT_EQ(1, GetProfile()->call_tree_size());

  // Add a partially constructed CallTree
  OnFunctionEntry();

  // Verify that the partially constructed CallTree extended the
  // CallTree vector.
  ASSERT_EQ(2, GetProfile()->call_tree_size());

  profile_->Stop();

  // Verify that stopping the profiler trimmed off the partially
  // constructed CallTree.
  ASSERT_EQ(1, GetProfile()->call_tree_size());
}

TEST_F(CallGraphProfileTest, ShouldIncludeInProfile) {
  ASSERT_TRUE(profile_->ShouldIncludeInProfile("http://example.com/foo.js"));
  ASSERT_TRUE(profile_->ShouldIncludeInProfile("http://example.com/"));
  ASSERT_TRUE(profile_->ShouldIncludeInProfile("http://example.com/bar.html"));
}

TEST_F(CallGraphProfileTest, ShouldNotIncludeInProfile) {
  ASSERT_FALSE(profile_->ShouldIncludeInProfile("about:/foo.js"));
  ASSERT_FALSE(profile_->ShouldIncludeInProfile("about:/index.html"));
  ASSERT_FALSE(profile_->ShouldIncludeInProfile("about:/foo/"));

  ASSERT_FALSE(profile_->ShouldIncludeInProfile("chrome:/foo.js"));
  ASSERT_FALSE(profile_->ShouldIncludeInProfile("chrome:/index.html"));
  ASSERT_FALSE(profile_->ShouldIncludeInProfile("chrome:/foo/"));

  ASSERT_FALSE(profile_->ShouldIncludeInProfile("file:/foo.js"));
  ASSERT_FALSE(profile_->ShouldIncludeInProfile("file:/index.html"));
  ASSERT_FALSE(profile_->ShouldIncludeInProfile("file:/foo/"));

  ASSERT_FALSE(profile_->ShouldIncludeInProfile("javascript:/foo.js"));
  ASSERT_FALSE(profile_->ShouldIncludeInProfile("javascript:/index.html"));
  ASSERT_FALSE(profile_->ShouldIncludeInProfile("javascript:/foo/"));

  ASSERT_FALSE(profile_->ShouldIncludeInProfile("foo.cpp"));
  ASSERT_FALSE(profile_->ShouldIncludeInProfile("http://example.com/foo.cpp"));

  ASSERT_FALSE(profile_->ShouldIncludeInProfile("XStringBundle"));
}

TEST_F(CallGraphProfileTest, ShouldIncludeInProfileCornerCases) {
  // We don't actually expect to encounter URLs like these when
  // running, but to verify correctness of the ShouldIncludeInProfile
  // implementation, we include these tests here.
  ASSERT_TRUE(profile_->ShouldIncludeInProfile(""));
  ASSERT_TRUE(profile_->ShouldIncludeInProfile("XStringBundl"));
  ASSERT_TRUE(profile_->ShouldIncludeInProfile("XStringBundleZ"));

  ASSERT_TRUE(profile_->ShouldIncludeInProfile("about"));
  ASSERT_TRUE(profile_->ShouldIncludeInProfile("chrome"));
  ASSERT_TRUE(profile_->ShouldIncludeInProfile("file"));
  ASSERT_TRUE(profile_->ShouldIncludeInProfile("javascript"));
  ASSERT_TRUE(profile_->ShouldIncludeInProfile(".cp"));
  ASSERT_TRUE(profile_->ShouldIncludeInProfile("cpp"));
}

}  // namespace
