// Copyright 2010 and onwards Google Inc.
// Author: sligocki@google.com (Shawn Ligocki)

#include "net/instaweb/rewriter/public/file_input_resource.h"

#include "net/instaweb/util/public/file_system.h"
#include "net/instaweb/util/public/simple_meta_data.h"

namespace net_instaweb {

FileInputResource::FileInputResource(const std::string& url,
                                     const std::string& filename,
                                     FileSystem* file_system)
    : url_(url),
      filename_(filename),
      file_system_(file_system) {
}

FileInputResource::~FileInputResource() {
}

bool FileInputResource::Read(MessageHandler* message_handler) {
  if (!loaded() &&
      file_system_->ReadFile(filename_.c_str(), &contents_, message_handler)) {
    meta_data_.reset(new SimpleMetaData());
  }
  return meta_data_ != NULL;
}

const MetaData* FileInputResource::metadata() const {
  return meta_data_.get();
}
}
