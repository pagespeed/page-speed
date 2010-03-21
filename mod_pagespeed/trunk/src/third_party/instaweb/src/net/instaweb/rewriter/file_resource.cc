// Copyright 2010 and onwards Google Inc.
// Author: jmarantz@google.com (Joshua Marantz)

#include "net/instaweb/rewriter/public/file_resource.h"

#include "net/instaweb/htmlparse/public/file_system.h"
#include "net/instaweb/htmlparse/public/writer.h"

namespace net_instaweb {
FileResource::FileResource(const char* filename)
  : filename_(filename),
    loaded_(false) {
}

bool FileResource::Load(
    FileSystem* file_system, MessageHandler* message_handler) {
  if (!loaded_) {
    loaded_ = file_system->ReadFile(
        filename_.c_str(), &contents_, message_handler);
  }
  return loaded_;
}

bool FileResource::IsLoaded() const {
  return loaded_;
}

bool FileResource::Write(
    FileSystem* file_system, Writer* writer, MessageHandler* message_handler) {
  return writer->Write(contents_.data(), contents_.size(), message_handler);
}
}
