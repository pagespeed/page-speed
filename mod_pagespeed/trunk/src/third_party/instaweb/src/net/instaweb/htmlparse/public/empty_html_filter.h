// Copyright 2010 and onwards Google Inc.
// Author: jmarantz@google.com (Joshua Marantz)

#ifndef NET_INSTAWEB_HTMLPARSE_PUBLIC_EMPTY_HTML_FILTER_H_
#define NET_INSTAWEB_HTMLPARSE_PUBLIC_EMPTY_HTML_FILTER_H_

#include <string>
#include "net/instaweb/htmlparse/public/html_filter.h"

namespace net_instaweb {

// Base class for rewriting filters that don't need to be sure to
// override every filter method.  Other filters that need to be sure
// they override every method would derive directly from HtmlFilter.
class EmptyHtmlFilter : public HtmlFilter {
 public:
  EmptyHtmlFilter();
  virtual ~EmptyHtmlFilter();

  virtual void StartDocument();
  virtual void EndDocument();
  virtual void StartElement(HtmlElement* element);
  virtual void EndElement(HtmlElement* element);
  virtual void Cdata(HtmlCdataNode* cdata);
  virtual void Comment(HtmlCommentNode* comment);
  virtual void IEDirective(const std::string& directive);
  virtual void Characters(HtmlCharactersNode* characters);
  virtual void Directive(HtmlDirectiveNode* directive);
  virtual void Flush();
};

}  // namespace net_instaweb

#endif  // NET_INSTAWEB_HTMLPARSE_PUBLIC_EMPTY_HTML_FILTER_H_
