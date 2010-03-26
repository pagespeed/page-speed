// Copyright 2010 Google Inc.
// Author: sligocki@google.com (Shawn Ligocki)

#include "net/instaweb/rewriter/public/dummy_url_fetcher.h"

#include <assert.h>
#include <string>
#include "net/instaweb/htmlparse/public/message_handler.h"

namespace net_instaweb {

bool DummyUrlFetcher::FetchUrl(const std::string& url,
                               std::string* fetched_content,
                               MessageHandler* message_handler) {
  message_handler->FatalError("", -1, "DummyUrlFetcher used");
  return false;
}
}
