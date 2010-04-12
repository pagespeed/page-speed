// Copyright 2010 and onwards Google Inc.
// Author: jmarantz@google.com (Joshua Marantz)

#ifndef NET_INSTAWEB_UTIL_PUBLIC_ABSTRACT_MUTEX_H_
#define NET_INSTAWEB_UTIL_PUBLIC_ABSTRACT_MUTEX_H_

namespace net_instaweb {

// Abstract interface for implementing a mutex.
class AbstractMutex {
 public:
  virtual ~AbstractMutex();
  virtual void Lock() = 0;
  virtual void Unlock() = 0;
};

// Helper class for lexically scoped mutexing.
class ScopedMutex {
 public:
  explicit ScopedMutex(AbstractMutex* mutex) : mutex_(mutex) {
    mutex_->Lock();
  }

  ~ScopedMutex() {
    mutex_->Unlock();
  }
 private:
  AbstractMutex* mutex_;
};
}

#endif  // NET_INSTAWEB_UTIL_PUBLIC_ABSTRACT_MUTEX_H_
