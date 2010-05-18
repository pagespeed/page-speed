// Copyright 2010 and onwards Google Inc.
// Author: jmarantz@google.com (Joshua Marantz)

#ifndef NET_INSTAWEB_UTIL_PUBLIC_STRING_BUFFER_WRITER_H_
#define NET_INSTAWEB_UTIL_PUBLIC_STRING_BUFFER_WRITER_H_

#include "net/instaweb/util/public/writer.h"

namespace net_instaweb {

class StringBuffer;

// Writer implementation for directing HTML output to a StringBuffer.
class StringBufferWriter : public Writer {
 public:
  explicit StringBufferWriter(StringBuffer* str) : string_(str) { }
  virtual ~StringBufferWriter();
  virtual bool Write(const StringPiece& str, MessageHandler* message_handler);
  virtual bool Flush(MessageHandler* message_handler);
 private:
  StringBuffer* string_;
};

}  // namespace net_instaweb

#endif  // NET_INSTAWEB_UTIL_PUBLIC_STRING_BUFFER_WRITER_H_
