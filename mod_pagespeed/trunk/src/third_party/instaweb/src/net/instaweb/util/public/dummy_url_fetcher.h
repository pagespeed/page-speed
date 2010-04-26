// Copyright 2010 Google Inc.
// Author: sligocki@google.com (Shawn Ligocki)
//
// Dummy implementation that aborts if used (useful for tests).

#ifndef NET_INSTAWEB_UTIL_PUBLIC_DUMMY_URL_FETCHER_H_
#define NET_INSTAWEB_UTIL_PUBLIC_DUMMY_URL_FETCHER_H_

#include "net/instaweb/util/public/url_fetcher.h"

namespace net_instaweb {

class MessageHandler;

class DummyUrlFetcher : public UrlFetcher {
 public:
  virtual bool StreamingFetchUrl(const std::string& url,
                                 const MetaData& request_headers,
                                 MetaData* response_headers,
                                 Writer* fetched_content_writer,
                                 MessageHandler* message_handler);
};

}  // namespace net_instaweb

#endif  // NET_INSTAWEB_UTIL_PUBLIC_DUMMY_URL_FETCHER_H_
