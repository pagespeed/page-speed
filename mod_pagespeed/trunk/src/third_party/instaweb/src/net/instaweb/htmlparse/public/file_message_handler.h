// Copyright 2010 and onwards Google Inc.
// Author: jmarantz@google.com (Joshua Marantz)

#ifndef NET_INSTAWEB_HTMLPARSE_PUBLIC_FILE_MESSAGE_HANDLER_H_
#define NET_INSTAWEB_HTMLPARSE_PUBLIC_FILE_MESSAGE_HANDLER_H_

#include <stdio.h>
#include <string>
#include "net/instaweb/htmlparse/public/message_handler.h"

namespace net_instaweb {

// Message handler implementation for directing all parser error and
// warning messages to a file.
class FileMessageHandler : public MessageHandler {
 public:
  explicit FileMessageHandler(FILE* file);
  virtual void WarningV(
      const char* filename, int line, const char *msg, va_list args);
  virtual void ErrorV(
      const char* filename, int line, const char *msg, va_list args);
  void FatalErrorV(
      const char* filename, int line, const char* msg, va_list args);

 private:
  FILE* file_;
};
}

#endif  // NET_INSTAWEB_HTMLPARSE_PUBLIC_FILE_MESSAGE_HANDLER_H_
