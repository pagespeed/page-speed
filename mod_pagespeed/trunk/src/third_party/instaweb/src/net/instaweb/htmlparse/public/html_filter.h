// Copyright 2010 and onwards Google Inc.
// Author: jmarantz@google.com (Joshua Marantz)

#ifndef NET_INSTAWEB_HTMLPARSE_PUBLIC_HTML_FILTER_H_
#define NET_INSTAWEB_HTMLPARSE_PUBLIC_HTML_FILTER_H_

#include <string>
#include "net/instaweb/htmlparse/public/html_parser_types.h"

namespace net_instaweb {

class HtmlFilter {
 public:
  HtmlFilter();
  virtual ~HtmlFilter();

  // Starts a new document.  Filters should clear their state in this function,
  // as the same Filter instance may be used for multiple HTML documents.
  virtual void StartDocument() = 0;
  virtual void EndDocument() = 0;


  // When an HTML element is encountered during parsing, each filter's
  // StartElement method is called.  The HtmlElement lives for the entire
  // duration of the document.
  //
  // TODO(jmarantz): consider passing handles rather than pointers and
  // reference-counting them instead to save memory on long documents.
  virtual void StartElement(HtmlElement* element) = 0;
  virtual void EndElement(HtmlElement* element) = 0;

  // TODO(jmarantz): provide a mechanism to mutate cdata and characters.
  virtual void Cdata(const std::string& cdata) = 0;

  // Called with the comment text.  Does not include the comment
  // delimeter.
  virtual void Comment(const std::string& comment) = 0;

  // Called for an IE directive; typically used for CSS styling.
  // See http://msdn.microsoft.com/en-us/library/ms537512(VS.85).aspx
  virtual void IEDirective(const std::string& directive) = 0;

  virtual void Characters(const std::string& characters) = 0;
  virtual void IgnorableWhitespace(const std::string& whitespace) = 0;
  virtual void DocType(const std::string& name, const std::string& ext_id,
                       const std::string& sys_id) = 0;
  virtual void Directive(const std::string& text) = 0;

  // Notifies the Filter that a flush is occurring.  A filter that's
  // generating streamed output should flush at this time.  A filter
  // that's mutating elements can mutate any element seen since the
  // most recent flush; once an element is flushed it is already on
  // the wire to its destination and it's too late to mutate.  Flush
  // is initiated by an application calling HttpParse::Flush().
  virtual void Flush() = 0;
};
}

#endif  // NET_INSTAWEB_HTMLPARSE_PUBLIC_HTML_FILTER_H_
