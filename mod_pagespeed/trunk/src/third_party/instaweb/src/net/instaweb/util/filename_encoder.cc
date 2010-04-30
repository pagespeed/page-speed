// Copyright 2010 Google Inc. All Rights Reserved.
// Author: sligocki@google.com (Shawn Ligocki)

#include "net/instaweb/util/public/filename_encoder.h"

namespace net_instaweb {

FilenameEncoder::~FilenameEncoder() {
}

void FilenameEncoder::Encode(const std::string& filename_prefix,
                             const std::string& filename_ending,
                             std::string* encoded_filename) {
  // Default filename encoder does no encoding.
  *encoded_filename = StrCat(filename_prefix, filename_ending);
}

}  // namespace net_instaweb
