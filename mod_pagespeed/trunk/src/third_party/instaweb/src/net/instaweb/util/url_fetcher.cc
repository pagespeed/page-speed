// Copyright 2010 Google Inc.
// Author: sligocki@google.com (Shawn Ligocki)

#include "net/instaweb/util/public/url_fetcher.h"
#include "net/instaweb/util/public/simple_meta_data.h"
#include "net/instaweb/util/public/string_writer.h"

namespace net_instaweb {

UrlFetcher::~UrlFetcher() {
}

bool UrlFetcher::FetchUrl(const std::string& url, std::string* content,
                          MessageHandler* message_handler) {
  StringWriter writer(content);
  SimpleMetaData request_headers, response_headers;
  return StreamingFetchUrl(url, request_headers, &response_headers, &writer,
                           message_handler);
}

}  // namespace net_instaweb
