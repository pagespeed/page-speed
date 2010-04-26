// Copyright 2010 and onwards Google Inc.
// Author: jmarantz@google.com (Joshua Marantz)

#ifndef NET_INSTAWEB_REWRITER_PUBLIC_CACHE_EXTENDER_H_
#define NET_INSTAWEB_REWRITER_PUBLIC_CACHE_EXTENDER_H_

#include <vector>

#include "net/instaweb/rewriter/public/css_filter.h"
#include "net/instaweb/rewriter/public/rewrite_filter.h"
#include "net/instaweb/util/public/atom.h"
#include <string>

namespace net_instaweb {

class Hasher;
class ResourceManager;
class ResourceServer;

// Rewrites resources to extend their cache lifetime, encoding the
// content hash into the new URL to ensure we do not serve stale
// data.
class CacheExtender : public RewriteFilter {
 public:
  CacheExtender(const char* path_prefix, HtmlParse* html_parse,
                ResourceManager* resource_manager,
                Hasher* hasher, ResourceServer* resource_server);

  virtual void StartElement(HtmlElement* element);
  virtual bool Fetch(StringPiece resource, Writer* writer,
                     const MetaData& request_header,
                     MetaData* response_headers,
                     UrlAsyncFetcher* fetcher,
                     MessageHandler* message_handler,
                     UrlAsyncFetcher::Callback* callback);

 private:
  Atom s_href_;
  HtmlParse* html_parse_;
  ResourceManager* resource_manager_;
  Hasher* hasher_;
  CssFilter css_filter_;
  ResourceServer* resource_server_;
};

}  // namespace net_instaweb

#endif  // NET_INSTAWEB_REWRITER_PUBLIC_CACHE_EXTENDER_H_
