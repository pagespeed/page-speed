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
// Timer implementation.  See timer.h for details.

#include "timer.h"

namespace activity {

Timer::Timer(ClockInterface *clock, int64_t start_time_usec) :
    clock_(clock),
    reference_time_usec_(start_time_usec),
    last_time_usec_(start_time_usec) {
}

Timer::~Timer() {}

int64_t Timer::GetElapsedTimeUsec() {
  int64_t now_usec = clock_->GetCurrentTimeUsec();
  const int64_t duration_since_last_time_usec = now_usec - last_time_usec_;
  if (duration_since_last_time_usec < 0) {
    // The clock went backwards. Offset the reference time by the
    // amount we went backwards, in order to guarantee that we always
    // return monotonically increasing values.
    reference_time_usec_ += duration_since_last_time_usec;
  }
  last_time_usec_ = now_usec;
  return now_usec - reference_time_usec_;
}

}  // namespace activity
