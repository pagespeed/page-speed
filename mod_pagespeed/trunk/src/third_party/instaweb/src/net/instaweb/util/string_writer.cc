// Copyright 2010 and onwards Google Inc.
// Author: jmarantz@google.com (Joshua Marantz)

#include "net/instaweb/util/public/string_writer.h"

namespace net_instaweb {

StringWriter::~StringWriter() {
}

bool StringWriter::Write(
    const char* str, int len, MessageHandler* message_handler) {
  string_->append(str, len);
  return true;
}

bool StringWriter::Flush(MessageHandler* message_handler) {
  return true;
}
}
