// Copyright 2010 and onwards Google Inc.
// Author: jmarantz@google.com (Joshua Marantz)

#ifndef NET_INSTAWEB_REWRITER_PUBLIC_CSS_FILTER_H_
#define NET_INSTAWEB_REWRITER_PUBLIC_CSS_FILTER_H_

#include <vector>

#include "net/instaweb/htmlparse/public/html_parser_types.h"
#include "net/instaweb/htmlparse/public/html_element.h"
#include "net/instaweb/util/public/atom.h"
#include <string>

namespace net_instaweb {

class CssFilter {
 public:
  explicit CssFilter(HtmlParse* html_parse);

  // Examines an HTML element to determine if it's a CSS link,
  // extracting out the HREF and the media-type.
  bool ParseCssElement(
      HtmlElement* element, HtmlElement::Attribute** href, const char** media);

 private:
  Atom s_link_;
  Atom s_href_;
  Atom s_type_;
  Atom s_rel_;
  Atom s_media_;
};

}  // namespace net_instaweb

#endif  // NET_INSTAWEB_REWRITER_PUBLIC_CSS_FILTER_H_
