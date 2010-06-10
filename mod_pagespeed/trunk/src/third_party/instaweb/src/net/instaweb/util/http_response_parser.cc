// Copyright 2010 Google Inc. All Rights Reserved.
// Author: sligocki@google.com (Shawn Ligocki)

#include "net/instaweb/util/public/http_response_parser.h"

#include <stdio.h>
#include "net/instaweb/util/public/meta_data.h"
#include "net/instaweb/util/public/writer.h"
#include "net/instaweb/util/stack_buffer.h"

namespace net_instaweb {

bool HttpResponseParser::Parse(FILE* stream) {
  char buf[kStackBufferSize];
  int nread;
  while (ok_ && ((nread = fread(buf, 1, sizeof(buf), stream)) > 0)) {
    ParseChunk(StringPiece(buf, nread));
  }
  return ok_;
}

bool HttpResponseParser::ParseChunk(const StringPiece& data) {
  if (reading_headers_) {
    int consumed = response_headers_->ParseChunk(data, message_handler_);
    if (response_headers_->headers_complete()) {
      // In this chunk we may have picked up some of the body.
      // Before we move to the next buffer, send it to the output
      // stream.
      ok_ = writer_->Write(data.substr(consumed), message_handler_);
      reading_headers_ = false;
    }
  } else {
    ok_ = writer_->Write(data, message_handler_);
  }
  return ok_;
}

}  // namespace net_instaweb
