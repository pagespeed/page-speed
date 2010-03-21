// Copyright 2010 and onwards Google Inc.
// Author: sligocki@google.com (Shawn Ligocki)

#include "net/instaweb/rewriter/public/outline_resource.h"

#include "net/instaweb/htmlparse/public/file_system.h"
#include "net/instaweb/htmlparse/public/writer.h"

namespace net_instaweb {
OutlineResource::OutlineResource(const std::string& contents,
                                 const std::string& url,
                                 const std::string& filename,
                                 int id)
    : contents_(contents),
      url_(url),
      filename_(filename),
      resource_id_(id) {
}

bool OutlineResource::Write(FileSystem* file_system, Writer* writer,
                            MessageHandler* message_handler) {
  return writer->Write(contents_.data(), contents_.size(), message_handler);
}
}
