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
  std::string cmd("/usr/bin/wget --save-headers -q -O - '");
  cmd += url;
  cmd += "'";
  FILE* wget_stdout = popen(cmd.c_str(), "r");
  if (wget_stdout == NULL) {
    message_handler->Error(url.c_str(), 0, strerror(errno));
  } else {
    ret = true;
    char buf[kStackBufferSize];
    int nread;
    while (((nread = fread(buf, 1, sizeof(buf), wget_stdout)) > 0) && ret) {
      ret = fetch.ParseChunk(buf, nread);
    }
    fclose(wget_stdout);
  }
  return ret;
}

bool WgetUrlFetcher::Fetch::ParseChunk(const char* data, int size) {
  if (reading_headers_) {
    int consumed = response_headers_->ParseChunk(data, size, message_handler_);
    if (response_headers_->headers_complete()) {
      // In this chunk we may have picked up some of the body.
      // Before we move to the next buffer, send it to the output
      // stream.
      ok_ = writer_->Write(data + consumed, size - consumed, message_handler_);
      reading_headers_ = false;
    }
  } else {
    ok_ = writer_->Write(data, size, message_handler_);
  }
  return ok_;
}

}  // namespace net_instaweb
