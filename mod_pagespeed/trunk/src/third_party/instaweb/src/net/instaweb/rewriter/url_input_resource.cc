// Copyright 2010 and onwards Google Inc.
// Author: sligocki@google.com (Shawn Ligocki)

#include "net/instaweb/rewriter/public/url_input_resource.h"
#include "net/instaweb/util/public/simple_meta_data.h"
#include "net/instaweb/util/public/string_writer.h"
#include "net/instaweb/util/public/url_fetcher.h"

namespace net_instaweb {

UrlInputResource::UrlInputResource(const std::string& url,
                                   UrlFetcher* url_fetcher)
    : url_(url),
      meta_data_(NULL),
      url_fetcher_(url_fetcher) {
}

UrlInputResource::~UrlInputResource() {
  if (meta_data_ != NULL) {
    delete meta_data_;
  }
}

bool UrlInputResource::Read(MessageHandler* message_handler) {
  if (meta_data_ != NULL) {
    StringWriter writer(&contents_);

    // TODO(jmarantz): consider request_headers.  E.g. will we ever
    // get different resources depending on user-agent?
    const MetaData* request_headers = NULL;
    meta_data_ = url_fetcher_->StreamingFetchUrl(url_, request_headers, &writer,
                                                 message_handler);
  }
  return (meta_data_ != NULL);
}

const MetaData* UrlInputResource::metadata() const {
  return meta_data_;
}
}
