// Copyright 2010 and onwards Google Inc.
// Author: jmarantz@google.com (Joshua Marantz)

#ifndef NET_INSTAWEB_UTIL_PUBLIC_STRING_WRITER_H_
#define NET_INSTAWEB_UTIL_PUBLIC_STRING_WRITER_H_

#include <string>
#include "net/instaweb/util/public/writer.h"

namespace net_instaweb {

// Writer implementation for directing HTML output to a string.
class StringWriter : public Writer {
 public:
  explicit StringWriter(std::string* str) : string_(str) { }
  virtual ~StringWriter();
  virtual bool Write(const char* str, int len, MessageHandler* message_handler);
  virtual bool Flush(MessageHandler* message_handler);
 private:
  std::string* string_;
};
}

#endif  // NET_INSTAWEB_UTIL_PUBLIC_STRING_WRITER_H_
