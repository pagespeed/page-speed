// Copyright 2010 Google Inc.
// Author: sligocki@google.com (Shawn Ligocki)

#include "net/instaweb/util/public/dummy_url_fetcher.h"

#include <assert.h>
#include "net/instaweb/util/public/message_handler.h"
#include <string>

namespace net_instaweb {

bool DummyUrlFetcher::StreamingFetchUrl(const std::string& url,
                                        const MetaData& request_headers,
                                        MetaData* response_headers,
                                        Writer* fetched_content_writer,
                                        MessageHandler* message_handler) {
  message_handler->FatalError("", -1, "DummyUrlFetcher used");
  return false;
}

}  // namespace net_instaweb
