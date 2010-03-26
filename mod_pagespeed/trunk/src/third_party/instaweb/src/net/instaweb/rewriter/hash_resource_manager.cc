// Copyright 2010 and onwards Google Inc.
// Author: sligocki@google.com (Shawn Ligocki)

#include "net/instaweb/rewriter/public/hash_resource_manager.h"

#include <string>
#include "net/instaweb/rewriter/public/hasher.h"
#include "net/instaweb/rewriter/public/hash_output_resource.h"

namespace net_instaweb {

HashResourceManager::HashResourceManager(
    const std::string& file_prefix, const std::string& url_prefix,
    int num_shards, FileSystem* file_system, UrlFetcher* url_fetcher,
    Hasher* hasher)
    : FilenameResourceManager(file_prefix, url_prefix, num_shards,
                              file_system, url_fetcher),
      hasher_(hasher) {
}

OutputResource* HashResourceManager::CreateOutputResource(
    const std::string& suffix) {
  OutputResource* resource = new HashOutputResource(
      url_prefix_, file_prefix_, suffix, file_system_, hasher_);
  // Note: resource will be deleted by ~FilenameResourceManager destructor.
  output_resources_.push_back(resource);
  return resource;
}
}
