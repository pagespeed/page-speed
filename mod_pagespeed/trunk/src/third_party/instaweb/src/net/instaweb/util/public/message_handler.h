// Copyright 2010 and onwards Google Inc.
// Author: jmarantz@google.com (Joshua Marantz)

#ifndef NET_INSTAWEB_UTIL_PUBLIC_MESSAGE_HANDLER_H_
#define NET_INSTAWEB_UTIL_PUBLIC_MESSAGE_HANDLER_H_

#include <stdarg.h>

#include "net/instaweb/util/public/printf_format.h"

namespace net_instaweb {
class MessageHandler {
 public:
  virtual ~MessageHandler();
  virtual void InfoV(
      const char* filename, int line, const char *msg, va_list args) = 0;
  virtual void WarningV(
      const char* filename, int line, const char *msg, va_list args) = 0;
  virtual void ErrorV(
      const char* filename, int line, const char *msg, va_list args) = 0;
  virtual void FatalErrorV(
      const char* filename, int line, const char* msg, va_list args) = 0;

  virtual void Info(
      const char* filename, int line, const char *msg, ...)
      INSTAWEB_PRINTF_FORMAT(4, 5);
  virtual void Warning(
      const char* filename, int line, const char *msg, ...)
      INSTAWEB_PRINTF_FORMAT(4, 5);
  virtual void Error(
      const char* filename, int line, const char *msg, ...)
      INSTAWEB_PRINTF_FORMAT(4, 5);
  virtual void FatalError(
      const char* filename, int line, const char* msg, ...)
      INSTAWEB_PRINTF_FORMAT(4, 5);
};
}

#endif  // NET_INSTAWEB_UTIL_PUBLIC_MESSAGE_HANDLER_H_
