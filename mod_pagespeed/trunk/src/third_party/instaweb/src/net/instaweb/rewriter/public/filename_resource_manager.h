// Copyright 2010 and onwards Google Inc.
// Author: sligocki@google.com (Shawn Ligocki)

#ifndef NET_INSTAWEB_REWRITER_PUBLIC_FILENAME_RESOURCE_MANAGER_H_
#define NET_INSTAWEB_REWRITER_PUBLIC_FILENAME_RESOURCE_MANAGER_H_

#include <vector>
#include "net/instaweb/rewriter/public/resource_manager.h"
#include <string>

namespace net_instaweb {

class ContentType;
class FileSystem;
class FilenameEncoder;
class InputResource;
class OutputResource;
class MessageHandler;
class MetaData;
class UrlFetcher;

class FilenameResourceManager : public ResourceManager {
 public:
  FilenameResourceManager(const StringPiece& file_prefix,
                          const StringPiece& url_prefix,
                          const int num_shards,
                          const bool write_http_headers,
                          FileSystem* file_system,
                          FilenameEncoder* filename_encoder,
                          UrlFetcher* url_fetcher);

  virtual ~FilenameResourceManager();

  virtual OutputResource* GenerateOutputResource(const ContentType& type);
  virtual OutputResource* NamedOutputResource(const StringPiece& name,
                                              const ContentType& type);
  virtual InputResource* CreateInputResource(const StringPiece& url);

  virtual void SetDefaultHeaders(const ContentType& content_type,
                                 MetaData* header);

  virtual void set_base_dir(const StringPiece& dir) {
    dir.CopyToString(&base_dir_);
  }
  virtual void set_base_url(const StringPiece& url) {
    url.CopyToString(&base_url_);
  }

  virtual StringPiece url_prefix() const { return url_prefix_; }
  virtual StringPiece file_prefix() const { return file_prefix_; }

 protected:

  std::string file_prefix_;
  std::string url_prefix_;
  std::string base_dir_;  // Directory where resources will be found.
  std::string base_url_;  // URL for the incoming request
  int num_shards_;   // NYI: For server sharding of OutputResources.
  int resource_id_;  // Output resources are named with sequential id nums.
  bool write_http_headers_;
  std::vector<InputResource*> input_resources_;
  std::vector<OutputResource*> output_resources_;
  FileSystem* file_system_;
  FilenameEncoder* filename_encoder_;
  UrlFetcher* url_fetcher_;
};

}  // namespace net_instaweb

#endif  // NET_INSTAWEB_REWRITER_PUBLIC_FILENAME_RESOURCE_MANAGER_H_
