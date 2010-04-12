// Copyright 2010 and onwards Google Inc.
// Author: jmarantz@google.com (Joshua Marantz)

#ifndef NET_INSTAWEB_UTIL_PUBLIC_CACHE_INTERFACE_H_
#define NET_INSTAWEB_UTIL_PUBLIC_CACHE_INTERFACE_H_

#include <string>

namespace net_instaweb {

class MessageHandler;
class Writer;

// Abstract interface for a cache.
class CacheInterface {
 public:
  virtual ~CacheInterface();
  virtual bool Get(const std::string& key, Writer* writer,
                   MessageHandler* message_handler) = 0;
  virtual void Put(const std::string& key, const std::string& value) = 0;
  virtual void Delete(const std::string& key) = 0;
};
}

#endif  // NET_INSTAWEB_UTIL_PUBLIC_CACHE_INTERFACE_H_
