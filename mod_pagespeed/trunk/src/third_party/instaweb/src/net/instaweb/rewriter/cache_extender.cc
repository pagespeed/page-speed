// Copyright 2010 and onwards Google Inc.
// Author: jmarantz@google.com (Joshua Marantz)

#include "net/instaweb/rewriter/public/cache_extender.h"

#include <assert.h>
#include "net/instaweb/rewriter/public/input_resource.h"
#include "net/instaweb/rewriter/public/resource_manager.h"
#include "net/instaweb/rewriter/rewrite.pb.h"  // for ResourceUrl
#include "net/instaweb/htmlparse/public/html_parse.h"
#include "net/instaweb/htmlparse/public/html_element.h"
#include "net/instaweb/util/public/hasher.h"
#include "net/instaweb/util/public/message_handler.h"
#include <string>

namespace net_instaweb {
// TODO(jmarantz): consider factoring out the code that finds external resources

CacheExtender::CacheExtender(const char* filter_prefix, HtmlParse* html_parse,
                             ResourceManager* resource_manager,
                             Hasher* hasher)
    : RewriteFilter(filter_prefix),
      html_parse_(html_parse),
      resource_manager_(resource_manager),
      hasher_(hasher),
      css_filter_(html_parse) {
  s_href_ = html_parse->Intern("href");
}

void CacheExtender::StartElement(HtmlElement* element) {
  MessageHandler* message_handler = html_parse_->message_handler();
  HtmlElement::Attribute* href;
  const char* media;
  if (css_filter_.ParseCssElement(element, &href, &media) &&
      html_parse_->IsRewritable(element)) {
    InputResource* css_resource =
        resource_manager_->CreateInputResource(href->value(), message_handler);

    // TODO(jmarantz): create an output resource to generate a new url,
    // rather than doing the content-hashing here.
    if (css_resource->Read(message_handler)) {
      ResourceUrl resource_url;
      resource_url.set_origin_url(href->value());
      resource_url.set_content_hash(hasher_->Hash(css_resource->contents()));
      std::string url_safe_id;
      Encode(resource_url, &url_safe_id);
      std::string new_url = StrCat(
          resource_manager_->url_prefix(), filter_prefix_,
          prefix_separator(), url_safe_id);
      href->set_value(new_url.c_str());
    }
  }
}

bool CacheExtender::Fetch(StringPiece url_safe_id,
                          Writer* writer,
                          const MetaData& request_headers,
                          MetaData* response_headers,
                          UrlAsyncFetcher* fetcher,
                          MessageHandler* message_handler,
                          UrlAsyncFetcher::Callback* callback) {
  std::string url, decoded_resource;
  bool ret = false;
  ResourceUrl resource_url;
  if (Decode(url_safe_id, &resource_url)) {
    const std::string& url = resource_url.origin_url();
    fetcher->StreamingFetch(url, request_headers, response_headers,
                            writer, message_handler, callback);
    ret = true;
  } else {
    url_safe_id.CopyToString(&url);
    message_handler->Error(url.c_str(), 0, "Unable to decode resource string");
  }
  return ret;
}

}  // namespace net_instaweb
