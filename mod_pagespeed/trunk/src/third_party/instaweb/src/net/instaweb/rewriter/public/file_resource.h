// Copyright 2010 and onwards Google Inc.
// Author: jmarantz@google.com (Joshua Marantz)

#ifndef NET_INSTAWEB_REWRITER_PUBLIC_FILE_RESOURCE_H_
#define NET_INSTAWEB_REWRITER_PUBLIC_FILE_RESOURCE_H_

#include <string>
#include "net/instaweb/rewriter/public/resource.h"

namespace net_instaweb {

class FileResource : public Resource {
 public:
  explicit FileResource(const char* filename);
  virtual bool Load(FileSystem* file_system, MessageHandler* message_handler);
  virtual bool IsLoaded() const;
  virtual bool Write(
      FileSystem* file_system, Writer* writer, MessageHandler* message_handler);

 private:
  std::string filename_;
  std::string contents_;
  bool loaded_;
};
}

#endif  // NET_INSTAWEB_REWRITER_PUBLIC_FILE_RESOURCE_H_
