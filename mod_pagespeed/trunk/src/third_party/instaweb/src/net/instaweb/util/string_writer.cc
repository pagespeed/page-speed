// Copyright 2010 and onwards Google Inc.
// Author: jmarantz@google.com (Joshua Marantz)

#include "net/instaweb/util/public/string_writer.h"

namespace net_instaweb {

StringWriter::~StringWriter() {
}

bool StringWriter::Write(const StringPiece& str, MessageHandler* handler) {
  string_->append(str.data(), str.size());
  return true;
}

bool StringWriter::Flush(MessageHandler* message_handler) {
  return true;
}

}  // namespace net_instaweb
