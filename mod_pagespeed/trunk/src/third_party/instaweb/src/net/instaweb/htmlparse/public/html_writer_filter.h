// Copyright 2010 and onwards Google Inc.
// Author: jmarantz@google.com (Joshua Marantz)

#ifndef NET_INSTAWEB_HTMLPARSE_PUBLIC_HTML_WRITER_FILTER_H_
#define NET_INSTAWEB_HTMLPARSE_PUBLIC_HTML_WRITER_FILTER_H_

#include <string.h>
#include "net/instaweb/htmlparse/public/html_element.h"
#include "net/instaweb/htmlparse/public/html_filter.h"
#include <string>

namespace net_instaweb {

class HtmlWriterFilter : public HtmlFilter {
 public:
  explicit HtmlWriterFilter(HtmlParse* html_parse);

  void set_writer(Writer* writer) { writer_ = writer; }
  virtual ~HtmlWriterFilter();

  virtual void StartDocument();
  virtual void EndDocument();
  virtual void StartElement(HtmlElement* element);
  virtual void EndElement(HtmlElement* element);
  virtual void Cdata(const std::string& cdata);
  virtual void Comment(const std::string& comment);
  virtual void IEDirective(const std::string& directive);
  virtual void Characters(const std::string& characters);
  virtual void IgnorableWhitespace(const std::string& whitespace);
  virtual void DocType(const std::string& name, const std::string& ext_id,
                       const std::string& sys_id);
  virtual void Directive(const std::string& directive);
  virtual void Flush();

  void set_max_column(int max_column) { max_column_ = max_column; }

 private:
  void EmitBytes(const std::string& str) {EmitBytes(str.c_str(), str.size());}
  void EmitBytes(const char* str) {EmitBytes(str, strlen(str));}
  void EmitBytes(const char* str, int size);
  HtmlElement::CloseStyle GetCloseStyle(HtmlElement* element);

  // Escapes arbitrary text as HTML, e.g. turning & into &amp;.  If quoteChar
  // is non-zero, e.g. '"', then it would escape " as well.
  void EncodeBytes(const std::string& val, int quoteChar);

  HtmlParse* html_parse_;
  Writer* writer_;

  // Helps writer exploit shortcuts like <img .../> rather than writing
  // <img ...></img>.  At the end of StartElement, we defer writing the ">"
  // until we see what's coming next.  If it's the matching end_tag, then
  // we can emit />.  If something else comes first, then we have to
  // first emit the delayed ">" before continuing.
  HtmlElement* lazy_close_element_;

  Atom symbol_a_;
  Atom symbol_link_;
  Atom symbol_href_;
  Atom symbol_img_;
  Atom symbol_script_;
  Atom symbol_src_;
  Atom symbol_alt_;
  int column_;
  int max_column_;
  int write_errors_;
};

}  // namespace net_instaweb

#endif  // NET_INSTAWEB_HTMLPARSE_PUBLIC_HTML_WRITER_FILTER_H_
