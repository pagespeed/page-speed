// Copyright 2010 and onwards Google Inc.
// Author: jmarantz@google.com (Joshua Marantz)

#ifndef NET_INSTAWEB_REWRITER_PUBLIC_HASH_RESOURCE_MANAGER_H_
#define NET_INSTAWEB_REWRITER_PUBLIC_HASH_RESOURCE_MANAGER_H_

#include <string>
#include <vector>
#include "net/instaweb/rewriter/public/filename_resource_manager.h"

namespace net_instaweb {

class FileSystem;
class Hasher;
class InputResource;
class OutputResource;
class MessageHandler;
class UrlFetcher;

// Note: the inheritance is mostly to consolidate code.
class HashResourceManager : public FilenameResourceManager {
 public:
  explicit HashResourceManager(const std::string& file_prefix,
                               const std::string& url_prefix,
                               int num_shards,
                               FileSystem* file_system,
                               UrlFetcher* url_fetcher,
                               Hasher* hasher);

  virtual OutputResource* CreateOutputResource(const std::string& suffix);

 private:
  Hasher* hasher_;
};
}

#endif  // NET_INSTAWEB_REWRITER_PUBLIC_HASH_RESOURCE_MANAGER_H_
