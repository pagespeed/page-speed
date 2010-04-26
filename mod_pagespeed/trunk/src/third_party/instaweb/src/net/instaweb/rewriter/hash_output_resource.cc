// Copyright 2010 and onwards Google Inc.
// Author: sligocki@google.com (Shawn Ligocki)

#include "net/instaweb/rewriter/public/hash_output_resource.h"

#include <assert.h>

#include "net/instaweb/util/public/hasher.h"
#include "net/instaweb/util/public/http_dump_util.h"
#include "net/instaweb/util/public/file_system.h"

namespace net_instaweb {

HashOutputResource::HashOutputResource(const std::string& url_prefix,
                                       const std::string& filename_prefix,
                                       const std::string& suffix,
                                       const bool write_http_headers,
                                       const bool garble_filename,
                                       FileSystem* file_system,
                                       Hasher* hasher)
    : FilenameOutputResource("", "", write_http_headers, file_system),
      url_prefix_(url_prefix),
      filename_prefix_(filename_prefix),
      suffix_(suffix),
      garble_filename_(garble_filename),
      hasher_(hasher) {
  // Note: url is empty until we write contents of file.
}

bool HashOutputResource::StartWrite(MessageHandler* message_handler) {
  hasher_->Reset();
  return FilenameOutputResource::StartWrite(message_handler);
}

// Called by FilenameOutputResource::StartWrite to determine how
// to start writing the tmpfile.
std::string HashOutputResource::TempPrefix() const {
  return filename_prefix_ + "temp_";
}

bool HashOutputResource::WriteChunk(const char* data, size_t size,
                                    MessageHandler* handler) {
  hasher_->Add(data, size);
  return FilenameOutputResource::WriteChunk(data, size, handler);
}

bool HashOutputResource::EndWrite(MessageHandler* message_handler) {
  hasher_->ComputeHash(&hash_);
  url_ = url_prefix_ + hash_ + suffix_;

  filename_ = filename_prefix_;
  if (garble_filename_) {
    std::string ungarbled_end = hash_ + suffix_;
    // Appends garbled end.
    latencylab::EscapeNonAlphanum(ungarbled_end, &filename_);
  } else {
    filename_ += hash_;
    filename_ += suffix_;
  }

  return FilenameOutputResource::EndWrite(message_handler);
}

const std::string& HashOutputResource::url() const {
  if (url_.empty()) {
    // Error: tried to get url before Writing file.
    assert(false);
  }
  return url_;
}

}  // namespace net_instaweb
