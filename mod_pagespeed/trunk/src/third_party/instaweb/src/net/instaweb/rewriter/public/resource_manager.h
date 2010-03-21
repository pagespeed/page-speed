// Copyright 2010 and onwards Google Inc.
// Author: jmarantz@google.com (Joshua Marantz)

#ifndef NET_INSTAWEB_REWRITER_PUBLIC_RESOURCE_MANAGER_H_
#define NET_INSTAWEB_REWRITER_PUBLIC_RESOURCE_MANAGER_H_

#include <string>

namespace net_instaweb {
class OutlineResource;
class MessageHandler;
class Resource;
class SpriteResource;

class ResourceManager {
 public:
  virtual ~ResourceManager();

  virtual SpriteResource* CreateSprite(const char* file_extension) = 0;
  virtual OutlineResource* CreateOutlineResource(
      const std::string& contents, const char* file_extension) = 0;
  virtual Resource* CreateResource(const std::string& name) = 0;
  virtual bool WriteResource(Resource* resource, const char* filename,
                             MessageHandler* message_handler) = 0;
  virtual bool Load(Resource* resource, MessageHandler* message_handler) = 0;
};
}

#endif  // NET_INSTAWEB_REWRITER_PUBLIC_RESOURCE_MANAGER_H_
