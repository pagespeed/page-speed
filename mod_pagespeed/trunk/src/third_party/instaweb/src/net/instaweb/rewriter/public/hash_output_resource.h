// Copyright 2010 and onwards Google Inc.
// Author: sligocki@google.com (Shawn Ligocki)
//
// HashOutputResource constructs filenames/urls based on a hash of contents.
// NOTE: resource.url() is not valid until after resource.Write() is called.

#ifndef NET_INSTAWEB_REWRITER_PUBLIC_HASH_OUTPUT_RESOURCE_H_
#define NET_INSTAWEB_REWRITER_PUBLIC_HASH_OUTPUT_RESOURCE_H_

#include <string>
#include "net/instaweb/rewriter/public/output_resource.h"

namespace net_instaweb {

class FileSystem;
class Hasher;
class MessageHandler;

class HashOutputResource : public OutputResource {
 public:
  explicit HashOutputResource(const std::string& url_prefix,
                              const std::string& filename_prefix,
                              const std::string& suffix,
                              FileSystem* file_system,
                              Hasher* hasher);

  // Write complete resource. Multiple calls will overwrite.
  virtual bool Write(const std::string& content,
                     MessageHandler* message_handler);

  virtual const std::string& url() const;

 private:
  std::string url_prefix_;
  std::string filename_prefix_;
  std::string suffix_;
  std::string hash_;
  std::string url_;
  std::string filename_;
  FileSystem* file_system_;
  Hasher* hasher_;
};
}

#endif  // NET_INSTAWEB_REWRITER_PUBLIC_HASH_OUTPUT_RESOURCE_H_
