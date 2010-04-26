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
}

bool UrlInputResource::Read(MessageHandler* message_handler) {
  bool ret = true;
  if (!loaded()) {
    StringWriter writer(&contents_);

    // TODO(jmarantz): consider request_headers.  E.g. will we ever
    // get different resources depending on user-agent?
    SimpleMetaData request_headers;
    meta_data_.reset(new SimpleMetaData);
    ret = url_fetcher_->StreamingFetchUrl(
        url_, request_headers, meta_data_.get(), &writer, message_handler);
  }
  return ret;
}

bool UrlInputResource::ContentsValid() const {
  return (loaded() && meta_data_->status_code() == HttpStatus::OK);
}

}  // namespace net_instaweb
