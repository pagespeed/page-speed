// Copyright 2010 and onwards Google Inc.
// Author: sligocki@google.com (Shawn Ligocki)

#include "net/instaweb/rewriter/public/filename_output_resource.h"

#include "net/instaweb/htmlparse/public/file_system.h"

namespace net_instaweb {

FilenameOutputResource::FilenameOutputResource(const std::string& url,
                                               const std::string& filename,
                                               FileSystem* file_system)
  : url_(url),
    filename_(filename),
    file_system_(file_system) {
}

bool FilenameOutputResource::Write(const std::string& content,
                                   MessageHandler* message_handler) {
  return file_system_->WriteFile(filename_.c_str(), content, message_handler);
}
}
