// Copyright 2010 and onwards Google Inc.
// Author: jmarantz@google.com (Joshua Marantz)

#ifndef NET_INSTAWEB_UTIL_PUBLIC_STRING_UTIL_H_
#define NET_INSTAWEB_UTIL_PUBLIC_STRING_UTIL_H_


#include <assert.h>
#include "base/string_util.h"
#include "third_party/base64/base64.h"

namespace net_instaweb {

inline std::string IntegerToString(int i) {
  return IntToString(i);
}

inline void Web64Encode(const std::string& in, std::string* out) {
  *out = web64_encode(reinterpret_cast<const unsigned char*>(in.data()),
                      in.size());
}

inline bool Web64Decode(const std::string& in, std::string* out) {
  bool ret = web64_decode(in, out);
  return ret;
}
}

#endif  // NET_INSTAWEB_UTIL_PUBLIC_STRING_UTIL_H_
