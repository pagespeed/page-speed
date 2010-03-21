// Copyright 2010 and onwards Google Inc.
// Author: sligocki@google.com (Shawn Ligocki)

#ifndef NET_INSTAWEB_REWRITER_PUBLIC_OUTLINE_RESOURCE_H_
#define NET_INSTAWEB_REWRITER_PUBLIC_OUTLINE_RESOURCE_H_

#include <string>
#include "net/instaweb/rewriter/public/resource.h"

namespace net_instaweb {

class OutlineResource : public Resource {
 public:
  OutlineResource(const std::string& contents, const std::string& url,
                  const std::string& filename, int id);

  virtual bool Write(
      FileSystem* file_system, Writer* writer, MessageHandler* message_handler);

  // TODO(sligocki): What are these for?
  virtual bool Load(FileSystem* file_system, MessageHandler* message_handler) {
    return false; }
  virtual bool IsLoaded() const { return false; }

  const std::string& url() const { return url_; }
  const std::string& filename() const { return filename_; }

 private:
  std::string contents_;
  std::string url_;
  std::string filename_;
  int resource_id_;
};
}

#endif  // NET_INSTAWEB_REWRITER_PUBLIC_OUTLINE_RESOURCE_H_
