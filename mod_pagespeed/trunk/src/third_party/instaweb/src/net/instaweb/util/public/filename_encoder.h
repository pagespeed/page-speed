// Copyright 2010 Google Inc. All Rights Reserved.
// Author: sligocki@google.com (Shawn Ligocki)

#ifndef NET_INSTAWEB_UTIL_PUBLIC_FILENAME_ENCODER_H_
#define NET_INSTAWEB_UTIL_PUBLIC_FILENAME_ENCODER_H_

#include "net/instaweb/util/public/string_util.h"

namespace net_instaweb {

class FilenameEncoder {
 public:
  FilenameEncoder() {}
  virtual ~FilenameEncoder();

  virtual void Encode(const std::string& filename_prefix,
                      const std::string& filename_ending,
                      std::string* encoded_filename);
};

}  // namespace net_instaweb

#endif  // NET_INSTAWEB_UTIL_PUBLIC_FILENAME_ENCODER_H_
