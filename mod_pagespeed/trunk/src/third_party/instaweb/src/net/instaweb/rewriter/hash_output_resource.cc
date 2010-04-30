// Copyright 2010 and onwards Google Inc.
// Author: sligocki@google.com (Shawn Ligocki)

#include "net/instaweb/rewriter/public/hash_output_resource.h"

#include <assert.h>

#include "net/instaweb/util/public/hasher.h"
#include "net/instaweb/util/public/file_system.h"
#include "net/instaweb/util/public/filename_encoder.h"

namespace net_instaweb {

HashOutputResource::HashOutputResource(const std::string& url_prefix,
                                       const std::string& filename_prefix,
                                       const std::string& suffix,
                                       const bool write_http_headers,
                                       FileSystem* file_system,
                                       FilenameEncoder* filename_encoder,
                                       Hasher* hasher)
    : FilenameOutputResource("", "", write_http_headers, file_system),
      url_prefix_(url_prefix),
      filename_prefix_(filename_prefix),
      suffix_(suffix),
      filename_encoder_(filename_encoder),
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
  return StrCat(filename_prefix_, "temp_");
}

bool HashOutputResource::WriteChunk(const char* data, size_t size,
                                    MessageHandler* handler) {
  hasher_->Add(data, size);
  return FilenameOutputResource::WriteChunk(data, size, handler);
}

bool HashOutputResource::EndWrite(MessageHandler* message_handler) {
  hasher_->ComputeHash(&hash_);
  url_ = StrCat(url_prefix_, hash_, suffix_);

  std::string raw_ending = StrCat(hash_, suffix_);
  filename_encoder_->Encode(filename_prefix_, raw_ending, &filename_);

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
