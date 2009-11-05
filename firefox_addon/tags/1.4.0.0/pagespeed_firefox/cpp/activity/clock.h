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
// Clock interface and implementation. ClockInterface should be used
// in classes that want to get the current time, in order to make
// those classes more testable. In production, use the Clock
// implementation. In tests, use the MockClock implementation.

#ifndef CLOCK_H_
#define CLOCK_H_

#include <stdint.h>

#include "base/basictypes.h"

namespace activity {

class ClockInterface {
 public:
  ClockInterface();
  virtual ~ClockInterface();

  virtual int64_t GetCurrentTimeUsec() = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(ClockInterface);
};

// A real clock implementation that returns the current time.
class Clock : public ClockInterface {
 public:
  Clock();
  virtual ~Clock();

  virtual int64_t GetCurrentTimeUsec();

 private:
  DISALLOW_COPY_AND_ASSIGN(Clock);
};

}  // namespace activity


namespace activity_testing {

// Simple mock implementation that increments the clock on each call
// to GetCurrentTimeUsec(). Should be used only for testing.
class MockClock : public activity::ClockInterface {
 public:
  MockClock();
  virtual ~MockClock();

  virtual int64_t GetCurrentTimeUsec();

  int64_t current_time_usec_;

 private:
  DISALLOW_COPY_AND_ASSIGN(MockClock);
};

}  // namespace activity_testing

#endif  // CLOCK_H_
