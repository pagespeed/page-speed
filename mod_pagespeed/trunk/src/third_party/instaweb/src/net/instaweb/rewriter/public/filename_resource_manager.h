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
class InputResource;
class OutputResource;
class MessageHandler;
class MetaData;
class UrlFetcher;

class FilenameResourceManager : public ResourceManager {
 public:
  FilenameResourceManager(const std::string& file_prefix,
                          const std::string& url_prefix,
                          const int num_shards,
                          const bool write_http_headers,
                          const bool garble_filenames,
                          FileSystem* file_system,
                          UrlFetcher* url_fetcher);

  virtual ~FilenameResourceManager();

  virtual OutputResource* CreateOutputResource(const ContentType& content_type);
  virtual InputResource* CreateInputResource(const std::string& url);

  virtual void SetDefaultHeaders(const ContentType& content_type,
                                 MetaData* header);

  virtual void set_base_dir(const std::string& dir) { base_dir_ = dir; }
  virtual void set_base_url(const std::string& url) { base_url_ = url; }

  virtual const std::string& url_prefix() const { return url_prefix_; }
  virtual const std::string& file_prefix() const { return file_prefix_; }

 protected:

  std::string file_prefix_;
  std::string url_prefix_;
  std::string base_dir_;  // Directory where resources will be found.
  std::string base_url_;  // URL for the incoming request
  int num_shards_;   // NYI: For server sharding of OutputResources.
  int resource_id_;  // Output resources are named with sequential id nums.
  bool write_http_headers_;
  bool garble_filenames_;
  std::vector<InputResource*> input_resources_;
  std::vector<OutputResource*> output_resources_;
  FileSystem* file_system_;
  UrlFetcher* url_fetcher_;
};

}  // namespace net_instaweb

#endif  // NET_INSTAWEB_REWRITER_PUBLIC_FILENAME_RESOURCE_MANAGER_H_
