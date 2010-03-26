// Copyright 2010 Google Inc.
// Author: sligocki@google.com (Shawn Ligocki)
//
// UrlFetcher is an interface for fetching urls.

#ifndef NET_INSTAWEB_REWRITER_PUBLIC_URL_FETCHER_H_
#define NET_INSTAWEB_REWRITER_PUBLIC_URL_FETCHER_H_

#include <string>

namespace net_instaweb {

class MessageHandler;

class UrlFetcher {
 public:
  virtual ~UrlFetcher();

  virtual bool FetchUrl(const std::string& url,
                        std::string* fetched_content,
                        MessageHandler* message_handler) = 0;
};
}

#endif  // NET_INSTAWEB_REWRITER_PUBLIC_URL_FETCHER_H_
