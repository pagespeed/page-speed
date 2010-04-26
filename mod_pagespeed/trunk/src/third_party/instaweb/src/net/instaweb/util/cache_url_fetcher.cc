// Copyright 2010 Google Inc.
// Author: jmarantz@google.com (Joshua Marantz)

#include "net/instaweb/util/public/cache_url_fetcher.h"
#include "net/instaweb/util/public/http_cache.h"
#include "net/instaweb/util/public/simple_meta_data.h"
#include "net/instaweb/util/public/string_writer.h"
#include "net/instaweb/util/public/url_async_fetcher.h"

namespace net_instaweb {

namespace {

// Helper class to hold state for a single asynchronous fetch.  When
// the fetch is complete, we'll put the resource in the cache.
class AsyncFetch : public UrlAsyncFetcher::Callback {
 public:
  AsyncFetch(const std::string& url, HTTPCache* cache, MessageHandler* handler)
      : url_(url),
        writer_(&content_),
        http_cache_(cache),
        message_handler_(handler) {
  }

  virtual void Done(bool success) {
    if (success) {
      // TODO(jmarantz): allow configuration of whether we ignore
      // IsProxyCacheable, e.g. for content served from the same host
      if (response_headers_.IsCacheable() &&
          (http_cache_->Query(url_.c_str(), message_handler_) !=
           CacheInterface::kInTransit)) {
        http_cache_->Put(url_.c_str(), response_headers_, content_,
                         message_handler_);
      } else {
        // TODO(jmarantz): cache that this request is not cacheable
      }
    } else {
      // TODO(jmarantz): cache that this request is not fetchable
    }
    delete this;
  }

  void Start(UrlAsyncFetcher* fetcher, const MetaData& request_headers) {
    fetcher->StreamingFetch(url_, request_headers, &response_headers_,
                            &writer_, message_handler_, this);
  }

  std::string url_;
  std::string content_;
  StringWriter writer_;
  HTTPCache* http_cache_;
  SimpleMetaData response_headers_;
  MessageHandler* message_handler_;
};

}  // namespace

CacheUrlFetcher::~CacheUrlFetcher() {
}

bool CacheUrlFetcher::StreamingFetchUrl(
    const std::string& url, const MetaData& request_headers,
    MetaData* response_headers, Writer* writer, MessageHandler* handler) {
  bool ret = false;
  ret = http_cache_->Get(url.c_str(), response_headers, writer, handler);
  if (!ret) {
    if (sync_fetcher_ != NULL) {
      // We need to hang onto a copy of the data so we can shove it
      // into the cache, which currently lacks a streaming Put.
      std::string content;
      StringWriter string_writer(&content);
      if (sync_fetcher_->StreamingFetchUrl(
              url, request_headers, response_headers,
              &string_writer, handler)) {
        writer->Write(content.data(), content.size(), handler);
        if (response_headers->IsCacheable()) {
          http_cache_->Put(url.c_str(), *response_headers, content, handler);
        } else {
          // TODO(jmarantz): cache that this request is not cacheable
        }
        ret = true;
      } else {
        // TODO(jmarantz): cache that this request is not fetchable
      }
    } else {
      AsyncFetch* fetch = new AsyncFetch(url, http_cache_, handler);
      fetch->Start(async_fetcher_, request_headers);
    }
  }
  return ret;
}

}  // namespace net_instaweb
