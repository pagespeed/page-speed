// Copyright 2010 and onwards Google Inc.
// Author: jmarantz@google.com (Joshua Marantz)

#ifndef NET_INSTAWEB_UTIL_PUBLIC_MESSAGE_HANDLER_H_
#define NET_INSTAWEB_UTIL_PUBLIC_MESSAGE_HANDLER_H_

#include <stdarg.h>

#include "net/instaweb/util/public/printf_format.h"

namespace net_instaweb {

enum MessageType {
  kInfo,
  kWarning,
  kError,
  kFatal
};

class MessageHandler {
 public:
  virtual ~MessageHandler();

  // Log an info, warning, error or fatal error message.
  virtual void Message(MessageType type, const char* msg, ...)
      INSTAWEB_PRINTF_FORMAT(3, 4);
  virtual void MessageV(MessageType type, const char* msg, va_list args) = 0;

  // Log a message with a filename and line number attached.
  virtual void FileMessage(MessageType type, const char* filename, int line,
                           const char* msg, ...) INSTAWEB_PRINTF_FORMAT(5, 6);
  virtual void FileMessageV(MessageType type, const char* filename, int line,
                            const char* msg, va_list args) = 0;

  virtual const char* MessageTypeToString(const MessageType type) const;

  // Convenience functions for backwards compatibility.
  // TODO(sligocki): Make non-virtual or remove these.
  virtual void InfoV(
      const char* filename, int line, const char* msg, va_list args) {
    FileMessageV(kInfo, filename, line, msg, args);
  }
  virtual void WarningV(
      const char* filename, int line, const char* msg, va_list args) {
    FileMessageV(kWarning, filename, line, msg, args);
  }
  virtual void ErrorV(
      const char* filename, int line, const char* msg, va_list args) {
    FileMessageV(kError, filename, line, msg, args);
  }
  virtual void FatalErrorV(
      const char* filename, int line, const char* msg, va_list args) {
    FileMessageV(kFatal, filename, line, msg, args);
  }

  virtual void Info(const char* filename, int line, const char* msg, ...)
      INSTAWEB_PRINTF_FORMAT(4, 5);
  virtual void Warning(const char* filename, int line, const char* msg, ...)
      INSTAWEB_PRINTF_FORMAT(4, 5);
  virtual void Error(const char* filename, int line, const char* msg, ...)
      INSTAWEB_PRINTF_FORMAT(4, 5);
  virtual void FatalError(const char* filename, int line, const char* msg, ...)
      INSTAWEB_PRINTF_FORMAT(4, 5);
};

}  // namespace net_instaweb

#endif  // NET_INSTAWEB_UTIL_PUBLIC_MESSAGE_HANDLER_H_
