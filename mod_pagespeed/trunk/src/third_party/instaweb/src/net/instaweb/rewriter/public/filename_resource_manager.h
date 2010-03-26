// Copyright 2010 and onwards Google Inc.
// Author: sligocki@google.com (Shawn Ligocki)

#ifndef NET_INSTAWEB_REWRITER_PUBLIC_FILENAME_RESOURCE_MANAGER_H_
#define NET_INSTAWEB_REWRITER_PUBLIC_FILENAME_RESOURCE_MANAGER_H_

#include <string>
#include <vector>
#include "net/instaweb/rewriter/public/resource_manager.h"

namespace net_instaweb {

class FileSystem;
class InputResource;
class OutputResource;
class MessageHandler;
class UrlFetcher;

class FilenameResourceManager : public ResourceManager {
 public:
  FilenameResourceManager(const std::string& file_prefix,
                          const std::string& url_prefix,
                          int num_shards,
                          FileSystem* file_system,
                          UrlFetcher* url_fetcher);

  virtual ~FilenameResourceManager();

  virtual OutputResource* CreateOutputResource(const std::string& suffix);
  virtual InputResource* CreateInputResource(const std::string& url);

  virtual void set_base_dir(const std::string& dir) { base_dir_ = dir; }

 protected:
  std::string file_prefix_;
  std::string url_prefix_;
  std::string base_dir_;  // Directory where resources will be found.
  int num_shards_;   // NYI: For server sharding of OutputResources.
  int resource_id_;  // Output resources are named with sequential id nums.
  std::vector<InputResource*> input_resources_;
  std::vector<OutputResource*> output_resources_;
  FileSystem* file_system_;
  UrlFetcher* url_fetcher_;
};
}

#endif  // NET_INSTAWEB_REWRITER_PUBLIC_FILENAME_RESOURCE_MANAGER_H_
