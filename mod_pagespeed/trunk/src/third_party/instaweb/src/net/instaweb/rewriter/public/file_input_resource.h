// Copyright 2010 and onwards Google Inc.
// Author: sligocki@google.com (Shawn Ligocki)
//
// Input resource created based on a local file.

#ifndef NET_INSTAWEB_REWRITER_PUBLIC_FILE_INPUT_RESOURCE_H_
#define NET_INSTAWEB_REWRITER_PUBLIC_FILE_INPUT_RESOURCE_H_

#include <string>
#include "net/instaweb/rewriter/public/input_resource.h"

namespace net_instaweb {

class FileSystem;
class MessageHandler;

class FileInputResource : public InputResource {
 public:
  explicit FileInputResource(const std::string& url,
                             const std::string& filename,
                             FileSystem* file_system);

  // Read complete resource, content is stored in contents_.
  virtual bool Read(MessageHandler* message_handler);

  virtual const std::string& url() const { return url_; }
  virtual bool loaded() const { return loaded_; }
  // contents are only available when loaded()
  virtual const std::string& contents() const { return contents_; }

 private:
  std::string url_;
  std::string filename_;
  std::string contents_;
  bool loaded_;
  FileSystem* file_system_;
};
}

#endif  // NET_INSTAWEB_REWRITER_PUBLIC_FILE_INPUT_RESOURCE_H_
