// Copyright 2010 and onwards Google Inc.
// Author: jmarantz@google.com (Joshua Marantz)

#include "net/instaweb/util/public/message_handler.h"

#include <assert.h>

namespace net_instaweb {

MessageHandler::~MessageHandler() {
}

const char* MessageHandler::MessageTypeToString(const MessageType type) const {
  switch (type) {
    case kInfo:
      return "Info";
    case kWarning:
      return "Warning";
    case kError:
      return "Error";
    case kFatal:
      return "Fatal";
    default:
      assert(false);
      return "INVALID MessageType!";
  }
}

void MessageHandler::Message(MessageType type, const char* msg, ...) {
  va_list args;
  va_start(args, msg);
  MessageV(type, msg, args);
  va_end(args);
}

void MessageHandler::FileMessage(MessageType type, const char* file, int line,
                                 const char* msg, ...) {
  va_list args;
  va_start(args, msg);
  FileMessageV(type, file, line, msg, args);
  va_end(args);
}

void MessageHandler::Info(const char* file, int line, const char* msg, ...) {
  va_list args;
  va_start(args, msg);
  InfoV(file, line, msg, args);
  va_end(args);
}

void MessageHandler::Error(const char* file, int line, const char* msg, ...) {
  va_list args;
  va_start(args, msg);
  ErrorV(file, line, msg, args);
  va_end(args);
}

void MessageHandler::Warning(const char* file, int line, const char* msg, ...) {
  va_list args;
  va_start(args, msg);
  WarningV(file, line, msg, args);
  va_end(args);
}

void MessageHandler::FatalError(
    const char* file, int line, const char* msg, ...) {
  va_list args;
  va_start(args, msg);
  FatalErrorV(file, line, msg, args);
  va_end(args);
}

}  // namespace net_instaweb
