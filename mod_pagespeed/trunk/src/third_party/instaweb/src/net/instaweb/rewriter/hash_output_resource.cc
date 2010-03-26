// Copyright 2010 and onwards Google Inc.
// Author: sligocki@google.com (Shawn Ligocki)

#include "net/instaweb/rewriter/public/hash_output_resource.h"

#include <assert.h>

#include "net/instaweb/rewriter/public/hasher.h"
#include "net/instaweb/htmlparse/public/file_system.h"

namespace net_instaweb {

HashOutputResource::HashOutputResource(const std::string& url_prefix,
                                       const std::string& filename_prefix,
                                       const std::string& suffix,
                                       FileSystem* file_system,
                                       Hasher* hasher)
  : url_prefix_(url_prefix),
    filename_prefix_(filename_prefix),
    suffix_(suffix),
    file_system_(file_system),
    hasher_(hasher) {
  // Note: url is empty until we write contents of file.
}

bool HashOutputResource::Write(const std::string& content,
                               MessageHandler* message_handler) {
  hash_ = hasher_->Hash(content);
  url_ = url_prefix_ + hash_ + suffix_;
  filename_ = filename_prefix_ + hash_ + suffix_;

  return file_system_->WriteFile(filename_.c_str(), content, message_handler);
}

const std::string& HashOutputResource::url() const {
  if (url_.empty()) {
    // Error: tried to get url before Writing file.
    assert(false);
  }

  return url_;
}
}
