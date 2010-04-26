// Copyright 2010 and onwards Google Inc.
// Author: jmarantz@google.com (Joshua Marantz)

#ifndef NET_INSTAWEB_UTIL_PUBLIC_READER_H_
#define NET_INSTAWEB_UTIL_PUBLIC_READER_H_

namespace net_instaweb {

// Interface for reading bytes from an output stream.
class MessageHandler;
class Reader {
 public:
  virtual ~Reader();
  virtual bool Read(char* str, int len, MessageHandler* handler) = 0;
};

}  // namespace net_instaweb

#endif  // NET_INSTAWEB_UTIL_PUBLIC_READER_H_
