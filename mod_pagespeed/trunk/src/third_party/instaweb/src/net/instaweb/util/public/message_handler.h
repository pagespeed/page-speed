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

  // String representation for MessageType.
  virtual const char* MessageTypeToString(const MessageType type) const;

  // Log an info, warning, error or fatal error message.
  void Message(MessageType type, const char* msg, ...)
      INSTAWEB_PRINTF_FORMAT(3, 4);
  virtual void MessageV(MessageType type, const char* msg, va_list args) = 0;

  // Log a message with a filename and line number attached.
  void FileMessage(MessageType type, const char* filename, int line,
                   const char* msg, ...) INSTAWEB_PRINTF_FORMAT(5, 6);
  virtual void FileMessageV(MessageType type, const char* filename, int line,
                            const char* msg, va_list args) = 0;


  // Conditional errors.
  void Check(bool condition, const char* msg, ...) INSTAWEB_PRINTF_FORMAT(3, 4);
  void CheckV(bool condition, const char* msg, va_list args);


  // Convenience functions for FileMessage for backwards compatibility.
  // TODO(sligocki): Rename these to InfoAt, ... so that Info, ... can be used
  // for general Messages.
  void Info(const char* filename, int line, const char* msg, ...)
      INSTAWEB_PRINTF_FORMAT(4, 5);
  void Warning(const char* filename, int line, const char* msg, ...)
      INSTAWEB_PRINTF_FORMAT(4, 5);
  void Error(const char* filename, int line, const char* msg, ...)
      INSTAWEB_PRINTF_FORMAT(4, 5);
  void FatalError(const char* filename, int line, const char* msg, ...)
      INSTAWEB_PRINTF_FORMAT(4, 5);

  void InfoV(const char* filename, int line, const char* msg, va_list args) {
    FileMessageV(kInfo, filename, line, msg, args);
  }
  void WarningV(const char* filename, int line, const char* msg, va_list a) {
    FileMessageV(kWarning, filename, line, msg, a);
  }
  void ErrorV(const char* filename, int line, const char* msg, va_list args) {
    FileMessageV(kError, filename, line, msg, args);
  }
  void FatalErrorV(const char* fname, int line, const char* msg, va_list a) {
    FileMessageV(kFatal, fname, line, msg, a);
  }
};

}  // namespace net_instaweb

#endif  // NET_INSTAWEB_UTIL_PUBLIC_MESSAGE_HANDLER_H_
