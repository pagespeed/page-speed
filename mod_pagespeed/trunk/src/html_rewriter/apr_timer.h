// Copyright 2010 and onwards Google Inc.
// Author: lsong@google.com (Libo Song)

#ifndef HTML_REWRITER_APR_TIMER_H_
#define HTML_REWRITER_APR_TIMER_H_

#include "net/instaweb/util/public/timer.h"

using net_instaweb::Timer;

namespace html_rewriter {

class AprTimer : public Timer {
 public:
  virtual ~AprTimer();
  // Returns number of milliseconds since 1970.
  virtual int64 NowMs() const;
};

}  // namespace html_rewriter

#endif  // HTML_REWRITER_APR_TIMER_H_
