// Copyright 2010 and onwards Google Inc.
// Author: jmarantz@google.com (Joshua Marantz)

#include "net/instaweb/util/public/file_message_handler.h"

#include <stdio.h>
#include <stdlib.h>

namespace net_instaweb {

FileMessageHandler::FileMessageHandler(FILE* file) : file_(file) {
}

void FileMessageHandler::MessageV(MessageType type, const char* msg,
                                  va_list args) {
  fprintf(file_, "%s: ", MessageTypeToString(type));
  vfprintf(file_, msg, args);
  fputc('\n', file_);

  if (type == kFatal) {
    abort();
  }
}

void FileMessageHandler::FileMessageV(MessageType type, const char* filename,
                                      int line, const char *msg, va_list args) {
  fprintf(file_, "%s: %s:%d: ", MessageTypeToString(type), filename, line);
  vfprintf(file_, msg, args);
  fputc('\n', file_);

  if (type == kFatal) {
    abort();
  }
}

}  // namespace net_instaweb
