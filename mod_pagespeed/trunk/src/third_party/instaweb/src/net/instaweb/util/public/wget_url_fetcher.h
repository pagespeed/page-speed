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
  virtual bool StreamingFetchUrl(const std::string& url,
                                 const MetaData& request_headers,
                                 MetaData* response_headers,
                                 Writer* writer,
                                 MessageHandler* message_handler);

  // Class to represent the state of a single fetch, including the
  // incremental, re-entrant parsing of the headers & body. It is exposed
  // here to allow sharing of this parsing state code with
  // wget_url_async_fetcher.cc.
  class Fetch {
   public:
    Fetch(MetaData* response_headers, Writer* writer, MessageHandler* handler)
        : reading_headers_(true),
          ok_(true),
          response_headers_(response_headers),
          writer_(writer),
          message_handler_(handler) {
    }

    bool ok() const { return ok_; }

    // Start a wget pipe based on the supplied URL and request headers.
    bool Start(const std::string& url, const MetaData& request_headers);

    // Read a chunk of wget output, populating response_headers and calling
    // wrier on output, returning true if the status is ok.
    bool ParseChunk(const char* buf, int size);

   private:
    bool reading_headers_;
    bool ok_;
    MetaData* response_headers_;
    Writer* writer_;
    MessageHandler* message_handler_;
  };

 private:
  friend class WgetUrlFetcherTest;

  // Entry-point provided for testing using static data from wget.
  bool ParseWgetStream(FILE* f,
                       MetaData* response_headers,
                       Writer* writer,
                       MessageHandler* message_handler);
};

}  // namespace net_instaweb

#endif  // NET_INSTAWEB_UTIL_PUBLIC_WGET_URL_FETCHER_H_
