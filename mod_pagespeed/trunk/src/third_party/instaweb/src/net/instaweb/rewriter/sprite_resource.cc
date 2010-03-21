// Copyright 2010 and onwards Google Inc.
// Author: jmarantz@google.com (Joshua Marantz)

#include "net/instaweb/rewriter/public/sprite_resource.h"

#include "net/instaweb/htmlparse/public/file_system.h"
#include "net/instaweb/htmlparse/public/writer.h"

namespace net_instaweb {
SpriteResource::SpriteResource(
    const std::string& url, const std::string& filename, int id)
    : url_(url),
      filename_(filename),
      loaded_(false),
      resource_id_(id) {
}

void SpriteResource::AddResource(Resource* resource) {
  resources_.push_back(resource);
}

bool SpriteResource::Load(
    FileSystem* file_system, MessageHandler* message_handler) {
  if (!loaded_) {
    loaded_ = true;
    for (size_t i = 0; i < resources_.size(); ++i) {
      loaded_ &= resources_[i]->Load(file_system, message_handler);
    }
  }
  return loaded_;
}

bool SpriteResource::IsLoaded() const {
  return loaded_;
}

bool SpriteResource::Write(
    FileSystem* file_system, Writer* writer, MessageHandler* message_handler) {
  bool ret = true;
  for (size_t i = 0; i < resources_.size(); ++i) {
    ret &= resources_[i]->Write(file_system, writer, message_handler);
  }
  return ret;
}
}
