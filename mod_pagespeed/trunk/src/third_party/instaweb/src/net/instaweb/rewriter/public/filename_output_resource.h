// Copyright 2010 and onwards Google Inc.
// Author: sligocki@google.com (Shawn Ligocki)

#ifndef NET_INSTAWEB_REWRITER_PUBLIC_FILENAME_OUTPUT_RESOURCE_H_
#define NET_INSTAWEB_REWRITER_PUBLIC_FILENAME_OUTPUT_RESOURCE_H_

#include <string>
#include "net/instaweb/rewriter/public/output_resource.h"

namespace net_instaweb {

class FileSystem;
class MessageHandler;

class FilenameOutputResource : public OutputResource {
 public:
  explicit FilenameOutputResource(const std::string& url,
                                  const std::string& filename,
                                  FileSystem* file_system);

  // Write complete resource. Multiple calls will overwrite.
  virtual bool Write(const std::string& content,
                     MessageHandler* message_handler);

  virtual const std::string& url() const { return url_; }

 private:
  std::string url_;
  std::string filename_;
  FileSystem* file_system_;
};
}

#endif  // NET_INSTAWEB_REWRITER_PUBLIC_FILENAME_OUTPUT_RESOURCE_H_
