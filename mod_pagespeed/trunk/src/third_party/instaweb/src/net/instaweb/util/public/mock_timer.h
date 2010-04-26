// Copyright 2010 and onwards Google Inc.
// Author: jmarantz@google.com (Joshua Marantz)

#ifndef NET_INSTAWEB_UTIL_PUBLIC_MOCK_TIMER_H_
#define NET_INSTAWEB_UTIL_PUBLIC_MOCK_TIMER_H_

#include "net/instaweb/util/public/timer.h"

namespace net_instaweb {

class MockTimer : public Timer {
 public:
  explicit MockTimer(int64 time_ms) : time_ms_(time_ms) {}
  virtual ~MockTimer();

  // Returns number of milliseconds since 1970.
  virtual int64 NowMs() const;

  void set_time_ms(int64 time_ms) { time_ms_ = time_ms; }
  void advance_ms(int64 delta_ms) { time_ms_ += delta_ms; }

 private:
  int64 time_ms_;
};

}  // namespace net_instaweb

#endif  // NET_INSTAWEB_UTIL_PUBLIC_MOCK_TIMER_H_
