// Copyright 2010 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef MOD_PAGESPEED_PAGESPEED_PROCESS_CONTEXT_H_
#define MOD_PAGESPEED_PAGESPEED_PROCESS_CONTEXT_H_

#include "base/scoped_ptr.h"

// Forward declaration.
struct server_rec;

// Forward declaration.
namespace net_instaweb {
class FileSystem;
class MessageHandler;
class FileCache;
class HTTPCache;
class CacheUrlFetcher;
class CacheUrlAsyncFetcher;
class Timer;
}

namespace html_rewriter {

// Forward declaration.
class SerfUrlAsyncFetcher;
class AprTimer;


class PageSpeedProcessContext {
 public:
  PageSpeedProcessContext();
  ~PageSpeedProcessContext();
  SerfUrlAsyncFetcher* fetcher() const { return fetcher_.get(); }
  void set_fetcher(SerfUrlAsyncFetcher* fetcher);
  net_instaweb::FileSystem* file_system() const { return file_system_.get(); }
  void set_file_system(net_instaweb::FileSystem* file_system);
  net_instaweb::MessageHandler* message_handler() const {
    return message_handler_.get();
  }
  void set_message_handler(net_instaweb::MessageHandler* handler);
  net_instaweb::FileCache* file_cache() const { return file_cache_.get(); }
  void set_file_cache(net_instaweb::FileCache* file_cache);
  net_instaweb::Timer* timer() const { return timer_.get(); }
  void set_timer(net_instaweb::Timer* timer);
  net_instaweb::HTTPCache* http_cache() const { return http_cache_.get(); }
  void set_http_cache(net_instaweb::HTTPCache* http_cache);
  net_instaweb::CacheUrlFetcher* cache_url_fetcher() const {
    return cache_url_fetcher_.get();
  }
  void set_cache_url_fetcher(net_instaweb::CacheUrlFetcher* cache_url_fetcher);
  net_instaweb::CacheUrlAsyncFetcher* cache_url_async_fetcher() const {
    return cache_url_async_fetcher_.get();
  }
  void set_cache_url_async_fetcher(
      net_instaweb::CacheUrlAsyncFetcher* cache_url_async_fetcher);

 private:
  scoped_ptr<SerfUrlAsyncFetcher> fetcher_;
  scoped_ptr<net_instaweb::FileSystem> file_system_;
  scoped_ptr<net_instaweb::MessageHandler> message_handler_;
  scoped_ptr<net_instaweb::FileCache> file_cache_;
  scoped_ptr<net_instaweb::Timer> timer_;
  scoped_ptr<net_instaweb::HTTPCache> http_cache_;
  scoped_ptr<net_instaweb::CacheUrlFetcher> cache_url_fetcher_;
  scoped_ptr<net_instaweb::CacheUrlAsyncFetcher> cache_url_async_fetcher_;
};

const PageSpeedProcessContext* GetPageSpeedProcessContext(server_rec* server);
void CreatePageSpeedProcessContext(server_rec* server);

}  // namespace html_rewriter

#endif  // MOD_PAGESPEED_PAGESPEED_PROCESS_CONTEXT_H_
