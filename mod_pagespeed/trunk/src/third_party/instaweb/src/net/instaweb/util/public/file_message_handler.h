// Copyright 2010 and onwards Google Inc.
// Author: jmarantz@google.com (Joshua Marantz)

#ifndef NET_INSTAWEB_UTIL_PUBLIC_FILE_MESSAGE_HANDLER_H_
#define NET_INSTAWEB_UTIL_PUBLIC_FILE_MESSAGE_HANDLER_H_

#include <stdio.h>
#include "net/instaweb/util/public/message_handler.h"

namespace net_instaweb {

// Message handler implementation for directing all error and
// warning messages to a file.
class FileMessageHandler : public MessageHandler {
 public:
  explicit FileMessageHandler(FILE* file);

  virtual void MessageV(MessageType type, const char* msg, va_list args);

  virtual void FileMessageV(MessageType type, const char* filename, int line,
                    const char* msg, va_list args);

 private:
  FILE* file_;
};

}  // namespace net_instaweb

#endif  // NET_INSTAWEB_UTIL_PUBLIC_FILE_MESSAGE_HANDLER_H_
