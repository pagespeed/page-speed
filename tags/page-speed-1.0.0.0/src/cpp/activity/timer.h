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
// Timer should be used by classes that want to take snapshots of
// elapsed time relative to some base time.

#ifndef TIMER_H_
#define TIMER_H_

#include <stdint.h>

#include "base/macros.h"
#include "clock.h"

namespace activity {

class Timer {
 public:
  Timer(ClockInterface *clock, int64_t start_time_usec);
  ~Timer();

  /**
   * Get the number of microseconds that have passed since the timer
   * was initialized.  The values returned from GetElapsedTimeUsec are
   * guaranteed to be monotonically increasing.
   */
  int64_t GetElapsedTimeUsec();

 private:
  ClockInterface *const clock_;
  int64_t reference_time_usec_;
  int64_t last_time_usec_;

  DISALLOW_COPY_AND_ASSIGN(Timer);
};

}  // namespace activity

#endif  // TIMER_H_
