// Copyright 2010 and onwards Google Inc.
// Author: jmarantz@google.com (Joshua Marantz)

#ifndef NET_INSTAWEB_REWRITER_PUBLIC_REWRITE_FILTER_H_
#define NET_INSTAWEB_REWRITER_PUBLIC_REWRITE_FILTER_H_

#include "net/instaweb/htmlparse/public/empty_html_filter.h"
#include "net/instaweb/util/public/string_util.h"
#include "net/instaweb/util/public/url_async_fetcher.h"

namespace net_instaweb {

class UrlAsyncFetcher;
class Writer;
class RewriteFilter : public EmptyHtmlFilter {
 public:
  explicit RewriteFilter(StringPiece path_prefix)
      : path_prefix_(path_prefix.data(), path_prefix.size()) {
  }
  virtual ~RewriteFilter();

  // Fetches a resource written using the filter.  Filters that
  // encode all the data (URLs, meta-data) needed to reconstruct
  // a rewritten resource in a URL component, this method is the
  // mechanism for the filter to serve the rewritten resource.
  //
  // The flow is that a RewriteFilter is instantiated with
  // a path prefix, e.g. a two letter abbreviation of the
  // filter, like "ce" for CacheExtender.  When it rewrites a
  // resource, it replaces the href with a url constructed as
  //   HOST://PREFIX/ce/WEB64_ENCODED_PROTOBUF
  // The WEB64_ENCODED_PROTOBUF can then be decoded.  for
  // CacheExtender, the protobuf contains the content hash plus
  // the original URL.  For "ir" (ImgRewriterFilter) the protobuf
  // might include the original image URL, plus the pixel-dimensions
  // to which the image was resized.
  virtual bool Fetch(StringPiece resource_url, Writer* writer,
                     const MetaData& request_header,
                     MetaData* response_headers,
                     UrlAsyncFetcher* fetcher,
                     MessageHandler* message_handler,
                     UrlAsyncFetcher::Callback* callback) = 0;

 protected:
  std::string path_prefix_;  // Prefix that should be used in front of all
                              // rewritten URLS
};

}  // namespace net_instaweb

#endif  // NET_INSTAWEB_REWRITER_PUBLIC_REWRITE_FILTER_H_
