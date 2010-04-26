// Copyright 2010 and onwards Google Inc.
// Author: sligocki@google.com (Shawn Ligocki)

#include "net/instaweb/rewriter/public/hash_resource_manager.h"

#include "net/instaweb/rewriter/public/hash_output_resource.h"
#include "net/instaweb/util/public/content_type.h"
#include "net/instaweb/util/public/hasher.h"
#include <string>

namespace net_instaweb {

HashResourceManager::HashResourceManager(
    const std::string& file_prefix, const std::string& url_prefix,
    const int num_shards, const bool write_http_headers,
    const bool garble_filenames, FileSystem* file_system,
    UrlFetcher* url_fetcher, Hasher* hasher)
    : FilenameResourceManager(file_prefix, url_prefix, num_shards,
                              write_http_headers, garble_filenames,
                              file_system, url_fetcher),
      write_http_headers_(write_http_headers),
      garble_filenames_(garble_filenames),
      hasher_(hasher) {
}

OutputResource* HashResourceManager::CreateOutputResource(
    const ContentType& content_type) {
  OutputResource* resource = new HashOutputResource(
      url_prefix_, file_prefix_, content_type.file_extension(),
      write_http_headers_, garble_filenames_, file_system_, hasher_);
  SetDefaultHeaders(content_type, resource->metadata());
  // Note: resource will be deleted by ~FilenameResourceManager destructor.
  output_resources_.push_back(resource);
  return resource;
}

void HashResourceManager::SetDefaultHeaders(const ContentType& content_type,
                                            MetaData* header) {
  FilenameResourceManager::SetDefaultHeaders(content_type, header);
  header->Add("Cache", content_type.mime_type());
}

}  // namespace net_instaweb
