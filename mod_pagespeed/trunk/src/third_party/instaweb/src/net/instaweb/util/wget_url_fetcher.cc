// Copyright 2010 Google Inc.
// Author: jmarantz@google.com (Joshua Marantz)

#include "net/instaweb/util/public/wget_url_fetcher.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include "net/instaweb/util/public/message_handler.h"
#include "net/instaweb/util/public/writer.h"
#include "net/instaweb/util/public/simple_meta_data.h"

namespace net_instaweb {
WgetUrlFetcher::~WgetUrlFetcher() {
}

MetaData* WgetUrlFetcher::StreamingFetchUrl(const std::string& url,
                                            const MetaData* request_headers,
                                            Writer* writer,
                                            MessageHandler* message_handler) {
  MetaData* meta_data = NULL;
  std::string cmd("/usr/bin/wget --save-headers -O - ");
  cmd += url;
  FILE* f = popen(cmd.c_str(), "r");
  if (f == NULL) {
    message_handler->Error(url.c_str(), 0, strerror(errno));
  } else {
    meta_data = ParseWgetStream(f, writer, message_handler);
    fclose(f);
  }
  return meta_data;
}

MetaData* WgetUrlFetcher::ParseWgetStream(FILE* f, Writer* writer,
                                          MessageHandler* message_handler) {
  SimpleMetaData* meta_data = new SimpleMetaData();
  char buf[10000];
  int nread;
  bool ok = true;
  bool reading_headers = true;

  // Read the headers a line at a time out of the input stream.  In this
  // section we will be filling header_buf from the pipe, requesting another
  // chunk whenever we have not seen a line.
  std::string line_buf;
  do {
    nread = fread(buf, 1, sizeof(buf), f);
    if (nread > 0) {
      int num_consumed = meta_data->ParseChunk(buf, nread, message_handler);
      if (meta_data->headers_complete()) {
        // In this chunk we may have picked up some of the body.
        // Before we move to the next buffer, send it to the output
        // stream.
        ok = writer->Write(buf + num_consumed, nread - num_consumed,
                           message_handler);
        reading_headers = false;
      }
    }
  } while (reading_headers && (nread > 0));

  while (ok && ((nread = fread(buf, 1, sizeof(buf), f)) > 0)) {
    ok = writer->Write(buf, nread, message_handler);
  }
  fclose(f);
  if (!ok) {
    delete meta_data;
    meta_data = NULL;
  }
  return meta_data;
}
}
