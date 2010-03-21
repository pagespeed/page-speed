// Copyright 2010 and onwards Google Inc.
// Author: jmarantz@google.com (Joshua Marantz)

#ifndef NET_INSTAWEB_REWRITER_PUBLIC_SPRITE_RESOURCE_H_
#define NET_INSTAWEB_REWRITER_PUBLIC_SPRITE_RESOURCE_H_

#include <string>
#include <vector>
#include "net/instaweb/rewriter/public/resource.h"

namespace net_instaweb {

class SpriteResource : public Resource {
 public:
  SpriteResource(const std::string& url, const std::string& filename, int id);
  void AddResource(Resource* resource);

  virtual bool Load(FileSystem* file_system, MessageHandler* message_handler);
  virtual bool Write(
      FileSystem* file_system, Writer* writer, MessageHandler* message_handler);
  virtual bool IsLoaded() const;

  const std::string& url() const { return url_; }
  const std::string& filename() const { return filename_; }

 private:
  std::string url_;
  std::string filename_;
  std::vector<Resource*> resources_;
  bool loaded_;
  int resource_id_;
};
}

#endif  // NET_INSTAWEB_REWRITER_PUBLIC_SPRITE_RESOURCE_H_
