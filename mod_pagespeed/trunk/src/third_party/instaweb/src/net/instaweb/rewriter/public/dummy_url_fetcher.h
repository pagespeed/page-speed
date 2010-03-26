// Copyright 2010 Google Inc.
// Author: sligocki@google.com (Shawn Ligocki)
//
// Dummy implementation that aborts if used (useful for tests).

#ifndef NET_INSTAWEB_REWRITER_PUBLIC_DUMMY_URL_FETCHER_H_
#define NET_INSTAWEB_REWRITER_PUBLIC_DUMMY_URL_FETCHER_H_

#include <string>
#include "net/instaweb/rewriter/public/url_fetcher.h"

namespace net_instaweb {

class MessageHandler;

class DummyUrlFetcher : public UrlFetcher {
 public:
  bool FetchUrl(const std::string& url,
                std::string* fetched_content,
                MessageHandler* message_handler);
};
}

#endif  // NET_INSTAWEB_REWRITER_PUBLIC_DUMMY_URL_FETCHER_H_
