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

#include "timer.h"

#include "base/scoped_ptr.h"
#include "clock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

class TimerTest : public testing::Test {
 protected:
  virtual void SetUp() {
    clock_.reset(new activity_testing::MockClock());
  }

  scoped_ptr<activity_testing::MockClock> clock_;
};

TEST_F(TimerTest, DurationRelativeToStartTime) {
  clock_->current_time_usec_ = 100;
  activity::Timer timer(clock_.get(), clock_->GetCurrentTimeUsec());
  clock_->current_time_usec_ = 200;
  ASSERT_EQ(100, timer.GetElapsedTimeUsec());
}

TEST_F(TimerTest, DurationIsMonotonic) {
  clock_->current_time_usec_ = 100;
  activity::Timer timer(clock_.get(), clock_->GetCurrentTimeUsec());
  clock_->current_time_usec_ = 200;
  ASSERT_EQ(100, timer.GetElapsedTimeUsec());
  clock_->current_time_usec_ = 0;
  ASSERT_EQ(100, timer.GetElapsedTimeUsec());
  clock_->current_time_usec_ = 2;
  ASSERT_EQ(102, timer.GetElapsedTimeUsec());
  clock_->current_time_usec_ = 1;
  ASSERT_EQ(102, timer.GetElapsedTimeUsec());
  clock_->current_time_usec_ = 2;
  ASSERT_EQ(103, timer.GetElapsedTimeUsec());
  clock_->current_time_usec_ = 102;
  ASSERT_EQ(203, timer.GetElapsedTimeUsec());
}

}  // namespace
