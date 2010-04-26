// Copyright 2010 and onwards Google Inc.
// Author: jmarantz@google.com (Joshua Marantz)

#ifndef NET_INSTAWEB_REWRITER_PUBLIC_ADD_HEAD_FILTER_H_
#define NET_INSTAWEB_REWRITER_PUBLIC_ADD_HEAD_FILTER_H_

#include "net/instaweb/htmlparse/public/empty_html_filter.h"
#include "net/instaweb/util/public/atom.h"

namespace net_instaweb {

// Adds a 'head' element before the 'body', if none was found
// during parsing.  This enables downstream filters to assume
// that there will be a head.
class AddHeadFilter : public EmptyHtmlFilter {
 public:
  explicit AddHeadFilter(HtmlParse* parser);

  virtual void StartDocument();
  virtual void StartElement(HtmlElement* element);
  virtual void EndDocument();

 private:
  bool found_head_;
  Atom s_head_;
  Atom s_body_;
  HtmlParse* html_parse_;
};

}  // namespace net_instaweb

#endif  // NET_INSTAWEB_REWRITER_PUBLIC_ADD_HEAD_FILTER_H_
