// Copyright 2010 and onwards Google Inc.
// Author: jmarantz@google.com (Joshua Marantz)

#ifndef NET_INSTAWEB_REWRITER_PUBLIC_CSS_COMBINE_FILTER_H_
#define NET_INSTAWEB_REWRITER_PUBLIC_CSS_COMBINE_FILTER_H_

#include <vector>

#include "net/instaweb/rewriter/public/css_filter.h"
#include "net/instaweb/rewriter/public/rewrite_filter.h"
#include "net/instaweb/util/public/atom.h"
#include <string>

namespace net_instaweb {

class ResourceManager;

class CssCombineFilter : public RewriteFilter {
 public:
  CssCombineFilter(const char* path_prefix, HtmlParse* html_parse,
                  ResourceManager* resource_manager);

  virtual void StartDocument();
  virtual void StartElement(HtmlElement* element);
  virtual void EndElement(HtmlElement* element);
  virtual void Flush();
  virtual void IEDirective(const std::string& directive);
  virtual bool Fetch(StringPiece resource, Writer* writer,
                     const MetaData& request_header,
                     MetaData* response_headers,
                     UrlAsyncFetcher* fetcher,
                     MessageHandler* message_handler,
                     UrlAsyncFetcher::Callback* callback);

 private:
  void EmitCombinations();

  Atom s_head_;
  Atom s_type_;
  Atom s_link_;
  Atom s_href_;
  Atom s_rel_;
  Atom s_media_;

  std::vector<HtmlElement*> css_elements_;
  HtmlParse* html_parse_;
  HtmlElement* head_element_;  // Pointer to head element for future use
  ResourceManager* resource_manager_;
  CssFilter css_filter_;
};

}  // namespace net_instaweb

#endif  // NET_INSTAWEB_REWRITER_PUBLIC_CSS_COMBINE_FILTER_H_
