// Copyright 2010 and onwards Google Inc.
// Author: sligocki@google.com (Shawn Ligocki)

#include "net/instaweb/rewriter/public/url_input_resource.h"

#include "net/instaweb/rewriter/public/url_fetcher.h"

namespace net_instaweb {

UrlInputResource::UrlInputResource(const std::string& url,
                                   UrlFetcher* url_fetcher)
    : url_(url),
      url_fetcher_(url_fetcher) {
}

bool UrlInputResource::Read(MessageHandler* message_handler) {
  if (!loaded_) {  // We do not need to reload.
    loaded_ = url_fetcher_->FetchUrl(url_, &contents_, message_handler);
  }
  return loaded_;
}
}
