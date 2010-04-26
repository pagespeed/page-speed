// Copyright 2010 and onwards Google Inc.
// Author: jmarantz@google.com (Joshua Marantz)

#include "net/instaweb/rewriter/public/cache_extender.h"

#include <assert.h>
#include "net/instaweb/rewriter/public/input_resource.h"
#include "net/instaweb/rewriter/public/resource_manager.h"
#include "net/instaweb/rewriter/public/resource_server.h"
#include "net/instaweb/htmlparse/public/html_parse.h"
#include "net/instaweb/htmlparse/public/html_element.h"
#include "net/instaweb/util/public/hasher.h"
#include "net/instaweb/util/public/message_handler.h"
#include <string>

namespace net_instaweb {
// TODO(jmarantz): consider factoring out the code that finds external resources

CacheExtender::CacheExtender(const char* path_prefix, HtmlParse* html_parse,
                             ResourceManager* resource_manager,
                             Hasher* hasher, ResourceServer* resource_server)
    : RewriteFilter(path_prefix),
      html_parse_(html_parse),
      resource_manager_(resource_manager),
      hasher_(hasher),
      css_filter_(html_parse),
      resource_server_(resource_server) {
  s_href_ = html_parse->Intern("href");
}

void CacheExtender::StartElement(HtmlElement* element) {
  const char* href;
  const char* media;
  if (css_filter_.ParseCssElement(element, &href, &media) &&
      html_parse_->IsRewritable(element)) {
    InputResource* css_resource =
        resource_manager_->CreateInputResource(href);
    MessageHandler* message_handler = html_parse_->message_handler();
    if (css_resource->Read(message_handler)) {
      std::string url_safe_id;
      resource_server_->EncodeResource(css_resource->url().c_str(),
                                       css_resource->contents(),
                                       &url_safe_id);
      std::string new_url = resource_manager_->url_prefix() + path_prefix_ +
          "/" + url_safe_id;
      bool replaced = element->ReplaceAttribute(s_href_, new_url.c_str());
      assert(replaced);
    }
  }
}

bool CacheExtender::Fetch(StringPiece resource_url,
                          Writer* writer,
                          const MetaData& request_headers,
                          MetaData* response_headers,
                          UrlAsyncFetcher* fetcher,
                          MessageHandler* message_handler,
                          UrlAsyncFetcher::Callback* callback) {
  std::string url;
  bool ret = false;
  if (resource_server_->DecodeResource(resource_url, &url)) {
    fetcher->StreamingFetch(url, request_headers, response_headers,
                            writer, message_handler, callback);
    ret = true;
  } else {
    resource_url.CopyToString(&url);
    message_handler->Error(url.c_str(), 0, "Unable to decode resource string");
  }
  return ret;
}

}  // namespace net_instaweb
