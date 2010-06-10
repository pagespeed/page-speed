// Copyright 2010 Google Inc.
// Author: jmarantz@google.com (Joshua Marantz)

#ifndef NET_INSTAWEB_UTIL_PUBLIC_WGET_URL_FETCHER_H_
#define NET_INSTAWEB_UTIL_PUBLIC_WGET_URL_FETCHER_H_

#include <stdio.h>
#include <string>
#include "net/instaweb/util/public/url_fetcher.h"

namespace net_instaweb {

// Runs 'wget' via popen for blocking URL fetches.
class WgetUrlFetcher : public UrlFetcher {
 public:
  virtual ~WgetUrlFetcher();

  // TODO(sligocki): Allow protocol version number (e.g. HTTP/1.1)
  // and request type (e.g. GET, POST, etc.) to be specified.
  virtual bool StreamingFetchUrl(const std::string& url,
                                 const MetaData& request_headers,
                                 MetaData* response_headers,
                                 Writer* writer,
                                 MessageHandler* message_handler);

  // Default user agent to use.
  static const char kDefaultUserAgent[];
};

}  // namespace net_instaweb

#endif  // NET_INSTAWEB_UTIL_PUBLIC_WGET_URL_FETCHER_H_
