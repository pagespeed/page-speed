// Copyright 2010 and onwards Google Inc.
// Author: jmarantz@google.com (Joshua Marantz)

#include "net/instaweb/util/public/string_buffer_writer.h"
#include "net/instaweb/util/public/string_buffer.h"

namespace net_instaweb {

StringBufferWriter::~StringBufferWriter() {
}

bool StringBufferWriter::Write(const StringPiece& str,
                               MessageHandler* handler) {
  string_->Append(str);
  return true;
}

bool StringBufferWriter::Flush(MessageHandler* message_handler) {
  return true;
}

}  // namespace net_instaweb
