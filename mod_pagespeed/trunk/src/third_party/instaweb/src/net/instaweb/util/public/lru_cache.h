// Copyright 2010 and onwards Google Inc.
// Author: jmarantz@google.com (Joshua Marantz)

#ifndef NET_INSTAWEB_UTIL_PUBLIC_LRU_CACHE_H_
#define NET_INSTAWEB_UTIL_PUBLIC_LRU_CACHE_H_

#include <list>
#include <map>
#include "net/instaweb/util/public/cache_interface.h"
#include <string>

namespace net_instaweb {

// Simple C++ implementation of an in-memory least-recently used (LRU)
// cache.  This implementation is not thread-safe, and must be
// combined with a mutex to make it so.
//
// The purpose of this implementation is as a default implementation,
// or an local shadow for memcached.
//
// Also of note: the Get interface allows for streaming.  To get into
// a std::string, use a StringWriter.
//
// TODO(jmarantz): The Put interface does not currently stream, but this
// should be added.
class LRUCache : public CacheInterface {
 public:
  explicit LRUCache(size_t max_size)
      : max_bytes_in_cache_(max_size),
        current_bytes_in_cache_(0) { }
  virtual ~LRUCache();
  virtual bool Get(const std::string& key, Writer* writer,
                   MessageHandler* message_handler);
  virtual void Put(const std::string& key, const std::string& value,
                   MessageHandler* message_handler);
  virtual void Delete(const std::string& key,
                      MessageHandler* message_handler);

  // Determines the current state of a key.  In the case of an LRU
  // cache, objects are never kInTransit -- they are either kAvailable
  // or kNotFound.
  virtual KeyState Query(const std::string& key,
                         MessageHandler* message_handler);

  // Total size in bytes of keys and values stored.
  size_t size_bytes() const { return current_bytes_in_cache_; }

  // Number of elements stored
  size_t num_elements() const { return map_.size(); }

  // Sanity check the cache data structures.
  void SanityCheck();

 private:
  typedef std::pair<const std::string*, std::string> KeyValuePair;
  typedef std::list<KeyValuePair*> EntryList;
  // STL guarantees lifetime of list itererators as long as the node is in list.
  typedef EntryList::iterator ListNode;
  typedef std::map<std::string, ListNode> Map;

  // TODO(jmarantz): consider accounting for overhead for list cells, map
  // cells, string objects, etc.  Currently we are only accounting for the
  // actual characters in the key and value.
  int entry_size(KeyValuePair* kvp) const {
    return kvp->first->size() + kvp->second.size();
  }
  inline ListNode Freshen(KeyValuePair* key_value);
  bool EvictIfNecessary(size_t bytes_needed);

  size_t max_bytes_in_cache_;
  size_t current_bytes_in_cache_;
  EntryList lru_ordered_list_;
  Map map_;
};

}  // namespace net_instaweb

#endif  // NET_INSTAWEB_UTIL_PUBLIC_LRU_CACHE_H_
