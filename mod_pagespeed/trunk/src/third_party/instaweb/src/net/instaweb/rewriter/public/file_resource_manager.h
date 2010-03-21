// Copyright 2010 and onwards Google Inc.
// Author: jmarantz@google.com (Joshua Marantz)

#ifndef NET_INSTAWEB_REWRITER_PUBLIC_FILE_RESOURCE_MANAGER_H_
#define NET_INSTAWEB_REWRITER_PUBLIC_FILE_RESOURCE_MANAGER_H_

#include <string>
#include <vector>
#include "net/instaweb/rewriter/public/resource_manager.h"

namespace net_instaweb {
class OutlineResource;
class FileSystem;
class SpriteResource;

class FileResourceManager : public ResourceManager {
 public:
  FileResourceManager(const char* file_prefix, const char* server_prefix,
                      int num_shards, FileSystem* file_system);
  ~FileResourceManager();

  virtual SpriteResource* CreateSprite(const char* file_extension);
  virtual OutlineResource* CreateOutlineResource(
      const std::string& contents, const char* file_extension);
  virtual Resource* CreateResource(const std::string& url);
  virtual bool WriteResource(Resource* resource, const char* filename,
                             MessageHandler* message_handler);
  virtual bool Load(Resource*, MessageHandler* message_handler);

  void set_search_path(const std::string& path) { search_path_ = path; }

 private:
  std::string file_prefix_;
  std::string server_prefix_;
  std::string search_path_;
  int num_shards_;
  int resource_id_;
  std::vector<Resource*> resources_;
  FileSystem* file_system_;
};
}

#endif  // NET_INSTAWEB_REWRITER_PUBLIC_FILE_RESOURCE_MANAGER_H_
