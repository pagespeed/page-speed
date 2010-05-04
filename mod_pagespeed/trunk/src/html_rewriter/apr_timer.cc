// Copyright 2010 and onwards Google Inc.
// Author: lsong@google.com (Libo Song)

#include "html_rewriter/apr_timer.h"
#include "third_party/apache_httpd/include/apr_time.h"

namespace html_rewriter {

AprTimer::~AprTimer() {
}

int64 AprTimer::NowMs() const {
  // apr_time_now returns microseconds.
  return apr_time_now() / 1000;
}

}  // namespace html_rewriter
