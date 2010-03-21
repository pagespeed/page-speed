// Copyright 2010 and onwards Google Inc.
// Author: jmarantz@google.com (Joshua Marantz)

#include "net/instaweb/htmlparse/public/file_message_handler.h"

#include <stdio.h>
#include <stdlib.h>

namespace net_instaweb {

FileMessageHandler::FileMessageHandler(FILE* file) : file_(file) {
}

void FileMessageHandler::WarningV(
    const char* filename, int line, const char* msg, va_list args) {
  fprintf(file_, "Warning: %s:%d: ", filename, line);
  vfprintf(file_, msg, args);
  fputc('\n', file_);
}

void FileMessageHandler::ErrorV(
    const char* filename, int line, const char *msg, va_list args) {
  fprintf(file_, "Error: %s:%d: ", filename, line);
  vfprintf(file_, msg, args);
  fputc('\n', file_);
}

void FileMessageHandler::FatalErrorV(
    const char* filename, int line, const char* msg, va_list args) {
  fprintf(file_, "Fatal: %s:%d: ", filename, line);
  vfprintf(file_, msg, args);
  fputc('\n', file_);
  fflush(file_);
  abort();
}
}
