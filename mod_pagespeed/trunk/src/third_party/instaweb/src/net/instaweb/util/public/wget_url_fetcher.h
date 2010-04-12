// Copyright 2010 Google Inc.
// Author: jmarantz@google.com (Joshua Marantz)

#ifndef NET_INSTAWEB_UTIL_PUBLIC_WGET_URL_FETCHER_H_
#define NET_INSTAWEB_UTIL_PUBLIC_WGET_URL_FETCHER_H_

#include <string>
#include "net/instaweb/util/public/url_fetcher.h"

class WgetServiceClient;
class HTTPServer;

namespace net_instaweb {

class MessageHandler;
class Writer;

class WgetUrlFetcher : public UrlFetcher {
 public:
  virtual ~WgetUrlFetcher();
  virtual MetaData* StreamingFetchUrl(const std::string& url,
                                      const MetaData* request_headers,
                                      Writer* writer,
                                      MessageHandler* message_handler);

 private:
  // Entry-point provided for testing using static data from wget.
  friend class WgetUrlFetcherTest;
  MetaData* ParseWgetStream(FILE* f, Writer* writer,
                            MessageHandler* message_handler);
};
}

#endif  // NET_INSTAWEB_UTIL_PUBLIC_WGET_URL_FETCHER_H_
