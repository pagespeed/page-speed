// Copyright 2010 and onwards Google Inc.
// Author: jmarantz@google.com (Joshua Marantz)

#ifndef NET_INSTAWEB_UTIL_PUBLIC_HTTP_CACHE_H_
#define NET_INSTAWEB_UTIL_PUBLIC_HTTP_CACHE_H_

#include "net/instaweb/util/public/cache_interface.h"
#include <string>

namespace net_instaweb {

class CacheInterface;
class MessageHandler;
class MetaData;
class Timer;
class Writer;

// Implements HTTP caching semantics, including cache expiration and
// retention of the originally served cache headers.
class HTTPCache {
 public:
  explicit HTTPCache(CacheInterface* cache, Timer* timer)
      : cache_(cache),
        timer_(timer),
        force_caching_(false) {
  }

  bool Get(const char* key, MetaData* headers, Writer* writer,
           MessageHandler* message_handler);

  void Put(const char* key, const MetaData& headers,
           const std::string& content,
           MessageHandler* handler);

  CacheInterface::KeyState Query(const char* key, MessageHandler* handler);
  void set_force_caching(bool force) { force_caching_ = true; }

 private:
  bool IsCurrentlyValid(const MetaData& headers);

  CacheInterface* cache_;
  Timer* timer_;
  bool force_caching_;
};

}  // namespace net_instaweb

#endif  // NET_INSTAWEB_UTIL_PUBLIC_HTTP_CACHE_H_
