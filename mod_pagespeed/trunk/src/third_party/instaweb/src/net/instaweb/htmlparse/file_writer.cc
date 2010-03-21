// Copyright 2010 and onwards Google Inc.
// Author: jmarantz@google.com (Joshua Marantz)

#include "public/file_writer.h"

namespace net_instaweb {

FileWriter::FileWriter(FileSystem::OutputFile* f) : file_(f) {
}

bool FileWriter::Write(
    const char* str, int len, MessageHandler* message_handler) {
  return file_->Write(str, len, message_handler) == len;
}

bool FileWriter::Flush(MessageHandler* message_handler) {
  return file_->Flush(message_handler);
}
}
