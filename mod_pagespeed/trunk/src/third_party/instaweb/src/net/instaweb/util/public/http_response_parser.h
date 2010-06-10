// Copyright 2010 Google Inc. All Rights Reserved.
// Author: sligocki@google.com (Shawn Ligocki)

#ifndef NET_INSTAWEB_UTIL_PUBLIC_HTTP_RESPONSE_PARSER_H_
#define NET_INSTAWEB_UTIL_PUBLIC_HTTP_RESPONSE_PARSER_H_

#include "net/instaweb/util/public/string_util.h"

namespace net_instaweb {

class MessageHandler;
class MetaData;
class Writer;

// Helper class to fascilitate parsing a raw streaming HTTP response including
// headers and body.
class HttpResponseParser {
 public:
  HttpResponseParser(MetaData* response_headers, Writer* writer,
                     MessageHandler* handler)
      : reading_headers_(true),
        ok_(true),
        response_headers_(response_headers),
        writer_(writer),
        message_handler_(handler) {
  }

  // Read a chunk of HTTP response, populating response_headers and call
  // writer on output body, returning true if the status is ok.
  bool ParseChunk(const StringPiece& data);

  // Parse complete HTTP response from a FILE stream.
  bool Parse(FILE* stream);

  bool ok() const { return ok_; }

 private:
  bool reading_headers_;
  bool ok_;
  MetaData* response_headers_;
  Writer* writer_;
  MessageHandler* message_handler_;
};

}  // namespace net_instaweb

#endif  // NET_INSTAWEB_UTIL_PUBLIC_HTTP_RESPONSE_PARSER_H_
