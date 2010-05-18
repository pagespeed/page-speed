// Copyright 2010 and onwards Google Inc.
// Author: jmarantz@google.com (Joshua Marantz)

#ifndef NET_INSTAWEB_UTIL_PUBLIC_WRITER_H_
#define NET_INSTAWEB_UTIL_PUBLIC_WRITER_H_

#include <string>
#include "net/instaweb/util/public/string_util.h"

namespace net_instaweb {

class MessageHandler;

// Interface for writing bytes to an output stream.
class Writer {
 public:
  virtual ~Writer();

  virtual bool Write(const StringPiece& str, MessageHandler* handler) = 0;

  virtual bool Flush(MessageHandler* message_handler) = 0;

  // Deprecated old interface. Do not use for new code.
  bool Write(const char* str, int len, MessageHandler* handler) {
    return Write(StringPiece(str, len), handler);
  }
};

}  // namespace net_instaweb

#endif  // NET_INSTAWEB_UTIL_PUBLIC_WRITER_H_
