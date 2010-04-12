// Copyright 2010 and onwards Google Inc.
// Author: jmarantz@google.com (Joshua Marantz)

#include "net/instaweb/util/public/threadsafe_cache.h"
#include "net/instaweb/util/public/abstract_mutex.h"

namespace net_instaweb {

ThreadsafeCache::~ThreadsafeCache() {
}

bool ThreadsafeCache::Get(const std::string& key, Writer* writer,
                          MessageHandler* message_handler) {
  ScopedMutex mutex(mutex_);
  return cache_->Get(key, writer, message_handler);
}

void ThreadsafeCache::Put(const std::string& key, const std::string& value) {
  ScopedMutex mutex(mutex_);
  cache_->Put(key, value);
}

void ThreadsafeCache::Delete(const std::string& key) {
  ScopedMutex mutex(mutex_);
  cache_->Delete(key);
}
}
