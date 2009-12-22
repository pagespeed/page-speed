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
// Clock implementation.  See clock.h for details.

#include "clock.h"

#include <stddef.h>  // for NULL

#if defined(_WINDOWS)

#include <Windows.h>

#else

#include <sys/time.h>

#endif

namespace {

// Platform-specific implementations of GetCurrentTimeUsecImpl().

#if defined(_WINDOWS)

// Windows doesn't provide gettimeofday(), but does provide
// GetSystemTimeAsFileTime(), which returns a 100-nanosecond
// resolution timestamp.

int64 GetCurrentTimeUsecImpl() {
  const int64 EPOCH_DIFF_USEC = 11644473600000000ULL;

  FILETIME now;
  GetSystemTimeAsFileTime(&now);

  int64 retval = 0;

  // Set the high bits
  retval |= now.dwHighDateTime;
  retval <<= 32;

  // Set the low bits
  retval |= now.dwLowDateTime;

  // Convert from 100-nanosecond resolution (returned by
  // GetSystemTimeAsFileTime()) to microsecond resolution (expected by
  // the caller).
  retval /= 10;

  // Convert from ANSI/Windows time base (Jan 1 1601) to epoch time
  // base (Jan 1 1970).
  retval -= EPOCH_DIFF_USEC;

  return retval;
}

#else

int64 GetCurrentTimeUsecImpl() {
  timeval now;
  gettimeofday(&now, NULL);
  return now.tv_sec * 1000000LL + now.tv_usec;
}

#endif

}  // namespace

namespace activity {

ClockInterface::ClockInterface() {}

ClockInterface::~ClockInterface() {}

Clock::Clock() {}

Clock::~Clock() {}

int64 Clock::GetCurrentTimeUsec() {
  return GetCurrentTimeUsecImpl();
}

}  // namespace activity


namespace activity_testing {

MockClock::MockClock() : current_time_usec_(0) {}

MockClock::~MockClock() {}

int64 MockClock::GetCurrentTimeUsec() {
  return current_time_usec_++;
}

}  // namespace activity_testing
