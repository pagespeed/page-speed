// Copyright 2010 and onwards Google Inc.
// Author: jmarantz@google.com (Joshua Marantz)

#ifndef NET_INSTAWEB_UTIL_PUBLIC_TIMER_H_
#define NET_INSTAWEB_UTIL_PUBLIC_TIMER_H_

#include "base/basictypes.h"

namespace net_instaweb {

// Timer interface, made virtual so it can be mocked for tests.
class Timer {
 public:
  static const int64 kSecondMs;
  static const int64 kMinuteMs;
  static const int64 kHourMs;
  static const int64 kDayMs;
  static const int64 kWeekMs;
  static const int64 kMonthMs;
  static const int64 kYearMs;

  virtual ~Timer();

  // Returns number of milliseconds since 1970.
  virtual int64 NowMs() const = 0;

  // Allocates and returns a system timer, owned by caller.
  static Timer* NewSystemTimer();

  // Parses an arbitrary string into milliseconds since 1970
  static bool ParseTime(const char* time_str, int64* time_ms);
};

}  // namespace net_instaweb

#endif  // NET_INSTAWEB_UTIL_PUBLIC_TIMER_H_
