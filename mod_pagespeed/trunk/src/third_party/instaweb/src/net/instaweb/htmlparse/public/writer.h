// Copyright 2010 and onwards Google Inc.
// Author: jmarantz@google.com (Joshua Marantz)

#ifndef NET_INSTAWEB_HTMLPARSE_PUBLIC_WRITER_H_
#define NET_INSTAWEB_HTMLPARSE_PUBLIC_WRITER_H_

#include <string>

namespace net_instaweb {

// Filter for writing bytes to an output stream.
class MessageHandler;
class Writer {
 public:
  virtual ~Writer();
  virtual bool Write(
      const char* str, int len, MessageHandler* message_handler) = 0;
  virtual bool Flush(MessageHandler* message_handler) = 0;
};
}

#endif  // NET_INSTAWEB_HTMLPARSE_PUBLIC_WRITER_H_
