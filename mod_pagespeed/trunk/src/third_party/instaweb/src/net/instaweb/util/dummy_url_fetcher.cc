// Copyright 2010 Google Inc.
// Author: sligocki@google.com (Shawn Ligocki)

#include "net/instaweb/util/public/dummy_url_fetcher.h"

#include <assert.h>
#include <string>
#include "net/instaweb/util/public/message_handler.h"

namespace net_instaweb {

MetaData* DummyUrlFetcher::StreamingFetchUrl(const std::string& url,
                                             const MetaData* request_headers,
                                             Writer* fetched_content_writer,
                                             MessageHandler* message_handler) {
  message_handler->FatalError("", -1, "DummyUrlFetcher used");
  return NULL;
}
}
