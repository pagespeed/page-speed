// Copyright 2010 Google Inc.
// Author: jmarantz@google.com (Joshua Marantz)

#include "net/instaweb/util/public/wget_url_fetcher.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include "net/instaweb/util/public/message_handler.h"
#include "net/instaweb/util/public/writer.h"
#include "net/instaweb/util/public/simple_meta_data.h"
#include "net/instaweb/util/stack_buffer.h"

namespace {

// It turns out to be harder to quote in bash with single-quote
// than double-quote.  From man sh:
//
//   Single Quotes
//     Enclosing characters in single quotes preserves the literal meaning of
//     all the characters (except single quotes, making it impossible to put
//     single-quotes in a single-quoted string).
//
//   Double Quotes
//     Enclosing characters within double quotes preserves the literal meaning
//     of all characters except dollarsign ($), backquote (‘), and backslash
//     (\).  The backslash inside double quotes is historically weird, and
//     serves to quote only the following characters:
//           $ ‘ " \ <newline>.
//     Otherwise it remains literal.
//
// So we put double-quotes around most strings, after first escaping
// any of these characters:
const char kEscapeChars[] = "\"$`\\";
}

namespace net_instaweb {

WgetUrlFetcher::~WgetUrlFetcher() {
}

bool WgetUrlFetcher::StreamingFetchUrl(const std::string& url,
                                       const MetaData& request_headers,
                                       MetaData* response_headers,
                                       Writer* writer,
                                       MessageHandler* message_handler) {
  bool ret = false;
  Fetch fetch(response_headers, writer, message_handler);
  std::string cmd("/usr/bin/wget --save-headers -q -O -"), escaped_url;

  for (int i = 0, n = request_headers.NumAttributes(); i < n; ++i) {
    std::string escaped_name, escaped_value;

    BackslashEscape(request_headers.Name(i), kEscapeChars, &escaped_name);
    BackslashEscape(request_headers.Value(i), kEscapeChars, &escaped_value);
    cmd += StrCat(" --header=\"", escaped_name, ": ", escaped_value, "\"");
  }

  BackslashEscape(url, kEscapeChars, &escaped_url);
  cmd += StrCat(" \"", escaped_url, "\"");
  fprintf(stderr, "%s\n", cmd.c_str());
  FILE* wget_stdout = popen(cmd.c_str(), "r");
  if (wget_stdout == NULL) {
    message_handler->Error(url.c_str(), 0, strerror(errno));
  } else {
    ret = true;
    char buf[kStackBufferSize];
    int nread;
    while (((nread = fread(buf, 1, sizeof(buf), wget_stdout)) > 0) && ret) {
      ret = fetch.ParseChunk(StringPiece(buf, nread));
    }
    int exit_status = pclose(wget_stdout);
    if (exit_status != 0) {
      // The wget failed.  wget does not always (ever?) write appropriate
      // headers when it fails, so invent some.
      if (response_headers->status_code() == 0) {
        response_headers->set_major_version(1);
        response_headers->set_minor_version(0);
        response_headers->set_status_code(HttpStatus::BAD_REQUEST);
        response_headers->set_reason_phrase("Wget Failed");
        response_headers->ComputeCaching();
        writer->Write(std::string("wget failed: "), message_handler);
        writer->Write(url, message_handler);
      }
    }
  }
  return ret;
}

bool WgetUrlFetcher::Fetch::ParseChunk(const StringPiece& data) {
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
