// Copyright 2010 and onwards Google Inc.
// Author: jmarantz@google.com (Joshua Marantz)

#ifndef NET_INSTAWEB_UTIL_PUBLIC_PTHREAD_MUTEX_H_
#define NET_INSTAWEB_UTIL_PUBLIC_PTHREAD_MUTEX_H_

#include <pthread.h>
#include "net/instaweb/util/public/abstract_mutex.h"

namespace net_instaweb {

// Implementation of AbstractMutext for Pthread mutexes.
class PthreadMutex : public AbstractMutex {
 public:
  PthreadMutex();
  virtual ~PthreadMutex();
  virtual void Lock();
  virtual void Unlock();
 private:
  pthread_mutex_t mutex_;
};
}

#endif  // NET_INSTAWEB_UTIL_PUBLIC_PTHREAD_MUTEX_H_
