// Copyright 2010 and onwards Google Inc.
// Author: sligocki@google.com (Shawn Ligocki)

#include "net/instaweb/rewriter/public/output_resource.h"
#include "net/instaweb/util/public/writer.h"

namespace net_instaweb {

OutputResource::~OutputResource() {
}

// Deprecated interface for writing the output file in chunks.  To
// be removed soon.
bool OutputResource::StartWrite(MessageHandler* message_handler) {
  assert(writer_ == NULL);
  writer_ = BeginWrite(message_handler);
  return writer_ != NULL;
}

bool OutputResource::WriteChunk(const StringPiece& buf,
                                MessageHandler* handler) {
  return writer_->Write(buf, handler);
}

bool OutputResource::EndWrite(MessageHandler* message_handler) {
  return EndWrite(writer_, message_handler);
}

}  // namespace net_instaweb
