// Copyright 2010 and onwards Google Inc.
// Author: jmarantz@google.com (Joshua Marantz)

#include "net/instaweb/util/public/file_message_handler.h"

#include <stdio.h>
#include <stdlib.h>

namespace net_instaweb {

FileMessageHandler::FileMessageHandler(FILE* file) : file_(file) {
}

void FileMessageHandler::InfoV(
    const char* filename, int line, const char* msg, va_list args) {
  FileMessageV("Info", filename, line, msg, args);
}

void FileMessageHandler::WarningV(
    const char* filename, int line, const char* msg, va_list args) {
  FileMessageV("Warning", filename, line, msg, args);
}

void FileMessageHandler::ErrorV(
    const char* filename, int line, const char *msg, va_list args) {
  FileMessageV("Error", filename, line, msg, args);
}

void FileMessageHandler::FatalErrorV(
    const char* filename, int line, const char* msg, va_list args) {
  FileMessageV("Fatal", filename, line, msg, args);
  fflush(file_);
  abort();
}

void FileMessageHandler::FileMessageV(
    const char* kind,
    const char* filename, int line, const char *msg, va_list args) {
  fprintf(file_, "%s: %s:%d: ", kind, filename, line);
  vfprintf(file_, msg, args);
  fputc('\n', file_);
}
}
