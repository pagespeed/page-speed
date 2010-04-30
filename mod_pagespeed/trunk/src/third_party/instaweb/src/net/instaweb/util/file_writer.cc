// Copyright 2010 and onwards Google Inc.
// Author: jmarantz@google.com (Joshua Marantz)

#include "net/instaweb/util/public/file_writer.h"

namespace net_instaweb {

FileWriter::~FileWriter() {
}

bool FileWriter::Write(const char* str, int len, MessageHandler* handler) {
  return file_->Write(str, len, handler);
}

bool FileWriter::Flush(MessageHandler* message_handler) {
  return file_->Flush(message_handler);
}

}  // namespace net_instaweb
