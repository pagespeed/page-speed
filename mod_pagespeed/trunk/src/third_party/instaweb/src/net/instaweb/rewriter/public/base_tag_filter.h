// Copyright 2010 and onwards Google Inc.
// Author: jmarantz@google.com (Joshua Marantz)

#ifndef NET_INSTAWEB_REWRITER_PUBLIC_BASE_TAG_FILTER_H_
#define NET_INSTAWEB_REWRITER_PUBLIC_BASE_TAG_FILTER_H_

#include "net/instaweb/htmlparse/public/empty_html_filter.h"
#include "net/instaweb/util/public/atom.h"
#include <string>

namespace net_instaweb {

// Add this filter into the HtmlParse chain to add a base
// tag into the head section of an HTML document.
//
// e.g.
//
// HtmlParser* parser = ...
// ...
// BaseTagFilter* base_tag_filter = new BaseTagFilter(parser);
// parser->AddFilter(base_tag_filter);
// base_tag_filter->set_base("http://my_new_base.com");
// ...
// parser->StartParse()...
class BaseTagFilter : public EmptyHtmlFilter {
 public:
  explicit BaseTagFilter(HtmlParse* parser);

  virtual void StartDocument();
  virtual void StartElement(HtmlElement* element);
  virtual void EndElement(HtmlElement* element);

  void set_base_url(const std::string& url) { base_url_ = url; }

 private:
  Atom s_head_;
  Atom s_base_;
  Atom s_href_;
  HtmlElement* s_head_element_;
  bool found_base_tag_;
  std::string base_url_;
  HtmlParse* html_parse_;
};

}  // namespace net_instaweb

#endif  // NET_INSTAWEB_REWRITER_PUBLIC_BASE_TAG_FILTER_H_
