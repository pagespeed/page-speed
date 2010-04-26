// Copyright 2010 and onwards Google Inc.
// Author: jmarantz@google.com (Joshua Marantz)

#ifndef NET_INSTAWEB_UTIL_PUBLIC_WRITER_H_
#define NET_INSTAWEB_UTIL_PUBLIC_WRITER_H_

#include <string>

namespace net_instaweb {

// Interface for writing bytes to an output stream.
class MessageHandler;
class Writer {
 public:
  virtual ~Writer();

  virtual bool Write(const char* str, int len, MessageHandler* handler) = 0;

  bool Write(const std::string& str, MessageHandler* handler) {
    return Write(str.data(), str.length(), handler);
  }

  virtual bool Flush(MessageHandler* message_handler) = 0;
};

}  // namespace net_instaweb

#endif  // NET_INSTAWEB_UTIL_PUBLIC_WRITER_H_
