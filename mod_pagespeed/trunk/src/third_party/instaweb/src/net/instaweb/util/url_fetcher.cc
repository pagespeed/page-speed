// Copyright 2010 Google Inc.
// Author: sligocki@google.com (Shawn Ligocki)

#include "net/instaweb/util/public/url_fetcher.h"
#include "net/instaweb/util/public/meta_data.h"
#include "net/instaweb/util/public/string_writer.h"

namespace net_instaweb {
UrlFetcher::~UrlFetcher() {
}

bool UrlFetcher::FetchUrl(const std::string& url, std::string* content,
                               MessageHandler* message_handler) {
  StringWriter writer(content);
  MetaData* headers = StreamingFetchUrl(url, NULL, &writer, message_handler);
  bool ret = false;
  if (headers != NULL) {
    ret = true;
    delete headers;
  }
  return ret;
}
}
