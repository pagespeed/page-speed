// Copyright 2010 and onwards Google Inc.
// Author: jmarantz@google.com (Joshua Marantz)

#include "net/instaweb/htmlparse/public/message_handler.h"

namespace net_instaweb {
MessageHandler::~MessageHandler() {
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
}
