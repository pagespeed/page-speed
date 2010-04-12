// Copyright 2010 and onwards Google Inc.
// Author: sligocki@google.com (Shawn Ligocki)
//
// Filter to take explicit <style> and <script> tags and outline them to files.

#ifndef NET_INSTAWEB_REWRITER_PUBLIC_OUTLINE_FILTER_H_
#define NET_INSTAWEB_REWRITER_PUBLIC_OUTLINE_FILTER_H_

#include <string>

#include "net/instaweb/htmlparse/public/empty_html_filter.h"

namespace net_instaweb {
class ResourceManager;

class OutlineFilter : public HtmlFilter {
 public:
  OutlineFilter(HtmlParse* html_parse, ResourceManager* resource_manager,
                bool outline_styles, bool outline_scripts);

  virtual void StartDocument();

  virtual void StartElement(HtmlElement* element);
  virtual void EndElement(HtmlElement* element);

  virtual void Flush();

  // HTML Events we expect to be in <style> and <script> elements
  virtual void Characters(const std::string& characters);
  virtual void IgnorableWhitespace(const std::string& whitespace);

  // HTML Events we do not expect to be in <style> and <script> elements
  virtual void Comment(const std::string& comment);
  virtual void Cdata(const std::string& cdata);
  virtual void IEDirective(const std::string& directive);

  // Ignored HTML Events
  virtual void EndDocument() {}
  virtual void DocType(const std::string& name, const std::string& ext_id,
                       const std::string& sys_id) {}
  virtual void Directive(const std::string& text) {}

 private:
  void OutlineStyle(HtmlElement* element, const std::string& content);
  void OutlineScript(HtmlElement* element, const std::string& content);

  const char* s_link_;
  const char* s_script_;
  const char* s_style_;
  const char* s_rel_;
  const char* s_stylesheet_;
  const char* s_href_;
  const char* s_src_;
  // The style/script element we are in (if it hasn't been flushed), else NULL
  HtmlElement* inline_element_;
  std::string buffer_;  // Content since the open of a style/script element
  HtmlParse* html_parse_;
  ResourceManager* resource_manager_;
  bool outline_styles_;   // Should we outline styles?
  bool outline_scripts_;  // Should we outline scripts?
};
}

#endif  // NET_INSTAWEB_REWRITER_PUBLIC_OUTLINE_FILTER_H_
