// Copyright 2010 and onwards Google Inc.
// Author: jmarantz@google.com (Joshua Marantz)

#ifndef NET_INSTAWEB_UTIL_PUBLIC_THREADSAFE_CACHE_H_
#define NET_INSTAWEB_UTIL_PUBLIC_THREADSAFE_CACHE_H_

#include "net/instaweb/util/public/cache_interface.h"
#include <string>

namespace net_instaweb {

class MessageHandler;
class Writer;
class AbstractMutex;

// Composes a cache with a Mutex to form a threadsafe cache.
class ThreadsafeCache : public CacheInterface {
 public:
  ThreadsafeCache(CacheInterface* cache, AbstractMutex* mutex)
      : cache_(cache),
        mutex_(mutex) {
  }
  virtual ~ThreadsafeCache();
  virtual bool Get(const std::string& key, Writer* writer,
                   MessageHandler* message_handler);
  virtual void Put(const std::string& key, const std::string& value,
                   MessageHandler* message_handler);
  virtual void Delete(const std::string& key,
                      MessageHandler* message_handler);
  virtual KeyState Query(const std::string& key,
                         MessageHandler* message_handler);
 private:
  CacheInterface* cache_;
  AbstractMutex* mutex_;
};

}  // namespace net_instaweb

#endif  // NET_INSTAWEB_UTIL_PUBLIC_THREADSAFE_CACHE_H_
