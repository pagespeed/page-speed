// Copyright 2010 and onwards Google Inc.
// Author: jmarantz@google.com (Joshua Marantz)

#include "net/instaweb/util/public/timer.h"

#include <assert.h>
#include <sys/time.h>
#include <time.h>
#include "pagespeed/core/resource_util.h"

namespace net_instaweb {

class RealSystemTimer : public Timer {
 public:
  RealSystemTimer() {
  }

  virtual ~RealSystemTimer() {
  }

  virtual int64 NowMs() const {
    // TODO(jmarantz): use pagespeed library instead of replicating
    // this code.
    //
    // I had attempted to use this from the Chromium snapshot in pagespeed:
    //   src/third_party/chromium/src/base/time.h
    // but evidently that file is not currently built in the .gyp
    // file from pagespeed.
    //
    // double now = base::Time::NowFromSystemTime().ToDoubleT();

    struct timeval tv;
    struct timezone tz = { 0, 0 };  // UTC
    if (gettimeofday(&tv, &tz) != 0) {
      assert(0 == "Could not determine time of day");
    }
    return (tv.tv_sec * 1000) + (tv.tv_usec / 1000);
  }

 private:
};

Timer::~Timer() {
}

Timer* Timer::NewSystemTimer() {
  return new RealSystemTimer();
}

bool Timer::ParseTime(const char* time_str, int64* time_ms) {
  return pagespeed::resource_util::ParseTimeValuedHeader(time_str, time_ms);
}

}  // namespace net_instaweb
