// Copyright 2010 and onwards Google Inc.
// Author: jmarantz@google.com (Joshua Marantz)

#ifndef NET_INSTAWEB_REWRITER_PUBLIC_HASH_RESOURCE_MANAGER_H_
#define NET_INSTAWEB_REWRITER_PUBLIC_HASH_RESOURCE_MANAGER_H_

#include <vector>
#include "net/instaweb/rewriter/public/filename_resource_manager.h"
#include <string>

namespace net_instaweb {

class ContentType;
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
                               const int num_shards,
                               const bool write_http_headers,
                               const bool garble_filenames,
                               FileSystem* file_system,
                               UrlFetcher* url_fetcher,
                               Hasher* hasher);

  virtual OutputResource* CreateOutputResource(const ContentType& content_type);

  virtual void SetDefaultHeaders(const ContentType& content_type,
                                 MetaData* header);

 private:
  bool write_http_headers_;
  bool garble_filenames_;
  Hasher* hasher_;
};

}  // namespace net_instaweb

#endif  // NET_INSTAWEB_REWRITER_PUBLIC_HASH_RESOURCE_MANAGER_H_
