// Copyright 2010 and onwards Google Inc.
// Author: sligocki@google.com (Shawn Ligocki)

#include "net/instaweb/rewriter/public/file_input_resource.h"

#include "net/instaweb/htmlparse/public/file_system.h"

namespace net_instaweb {

FileInputResource::FileInputResource(const std::string& url,
                                     const std::string& filename,
                                     FileSystem* file_system)
    : url_(url),
      filename_(filename),
      file_system_(file_system) {
}

bool FileInputResource::Read(MessageHandler* message_handler) {
  if (!loaded_) {  // We do not need to reload.
    loaded_ = file_system_->ReadFile(filename_.c_str(),
                                     &contents_,
                                     message_handler);
  }
  return loaded_;
}
}
