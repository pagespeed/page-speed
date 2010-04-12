// Copyright 2010 and onwards Google Inc.
// Author: jmarantz@google.com (Joshua Marantz)

#include "net/instaweb/util/public/pthread_mutex.h"

namespace net_instaweb {

PthreadMutex::PthreadMutex() {
  pthread_mutex_init(&mutex_, NULL);
}

PthreadMutex::~PthreadMutex() {
  pthread_mutex_destroy(&mutex_);
}

void PthreadMutex::Lock() {
  pthread_mutex_lock(&mutex_);
}

void PthreadMutex::Unlock() {
  pthread_mutex_unlock(&mutex_);
}
}
