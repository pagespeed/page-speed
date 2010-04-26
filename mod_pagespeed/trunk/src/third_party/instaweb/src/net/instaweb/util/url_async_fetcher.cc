// Copyright 2010 Google Inc.
// Author: sligocki@google.com (Shawn Ligocki)

#include "net/instaweb/util/public/url_async_fetcher.h"
#include "net/instaweb/util/public/meta_data.h"
#include "net/instaweb/util/public/string_writer.h"

namespace net_instaweb {

UrlAsyncFetcher::~UrlAsyncFetcher() {
}

UrlAsyncFetcher::Callback::~Callback() {
}

}  // namespace instaweb
