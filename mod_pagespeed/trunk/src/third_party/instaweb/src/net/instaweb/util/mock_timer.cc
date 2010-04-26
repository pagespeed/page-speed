// Copyright 2010 and onwards Google Inc.
// Author: jmarantz@google.com (Joshua Marantz)

#include "net/instaweb/util/public/mock_timer.h"

namespace net_instaweb {

MockTimer::~MockTimer() {
}

int64 MockTimer::NowMs() const {
  return time_ms_;
}

}  // namespace net_instaweb
