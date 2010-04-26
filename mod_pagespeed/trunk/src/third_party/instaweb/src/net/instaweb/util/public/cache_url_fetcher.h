// Copyright 2010 Google Inc.
// Author: jmarantz@google.com (Joshua Marantz)

#ifndef NET_INSTAWEB_UTIL_PUBLIC_CACHE_URL_FETCHER_H_
#define NET_INSTAWEB_UTIL_PUBLIC_CACHE_URL_FETCHER_H_

#include "base/scoped_ptr.h"
#include <string>
#include "net/instaweb/util/public/url_fetcher.h"

namespace net_instaweb {

class HTTPCache;
class MessageHandler;
class UrlAsyncFetcher;

// Composes a URL fetcher with an http cache, to generate a caching
// URL fetcher.
//
// This fetcher will return true and provide an immediate result for
// entries in the cache.  When entries are not in the cache, it will
// initiate an asynchronous 'get' and store the result in the cache.
//
// See also CacheUrlAsyncFetcher, which will yield its results asynchronously
// for elements not in the cache, and immediately for results that are.
class CacheUrlFetcher : public UrlFetcher {
 public:
  CacheUrlFetcher(HTTPCache* cache, UrlFetcher* fetcher)
      : http_cache_(cache),
        sync_fetcher_(fetcher),
        async_fetcher_(NULL) {
  }
  CacheUrlFetcher(HTTPCache* cache, UrlAsyncFetcher* fetcher)
      : http_cache_(cache),
        sync_fetcher_(NULL),
        async_fetcher_(fetcher) {
  }
  virtual ~CacheUrlFetcher();

  virtual bool StreamingFetchUrl(
      const std::string& url,
      const MetaData& request_headers,
      MetaData* response_headers,
      Writer* fetched_content_writer,
      MessageHandler* message_handler);

 private:
  HTTPCache* http_cache_;
  UrlFetcher* sync_fetcher_;
  UrlAsyncFetcher* async_fetcher_;
};

}  // namespace net_instaweb

#endif  // NET_INSTAWEB_UTIL_PUBLIC_CACHE_URL_FETCHER_H_
