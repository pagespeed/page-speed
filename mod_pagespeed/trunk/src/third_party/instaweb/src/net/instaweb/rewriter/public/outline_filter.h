// Copyright 2010 and onwards Google Inc.
// Author: sligocki@google.com (Shawn Ligocki)

#ifndef NET_INSTAWEB_REWRITER_PUBLIC_OUTLINE_FILTER_H_
#define NET_INSTAWEB_REWRITER_PUBLIC_OUTLINE_FILTER_H_

#include "net/instaweb/htmlparse/public/empty_html_filter.h"
#include "net/instaweb/util/public/atom.h"
#include <string>

namespace net_instaweb {

class OutputResource;
class MessageHandler;
class MetaData;
class ResourceManager;

// Filter to take explicit <style> and <script> tags and outline them to files.
class OutlineFilter : public HtmlFilter {
 public:
  OutlineFilter(HtmlParse* html_parse, ResourceManager* resource_manager,
                bool outline_styles, bool outline_scripts);

  virtual void StartDocument();

  virtual void StartElement(HtmlElement* element);
  virtual void EndElement(HtmlElement* element);

  virtual void Flush();

  // HTML Events we expect to be in <style> and <script> elements.
  virtual void Characters(HtmlCharactersNode* characters);

  // HTML Events we do not expect to be in <style> and <script> elements.
  virtual void Comment(HtmlCommentNode* comment);
  virtual void Cdata(HtmlCdataNode* cdata);
  virtual void IEDirective(const std::string& directive);

  // Ignored HTML Events.
  virtual void EndDocument() {}
  virtual void Directive(HtmlDirectiveNode* directive) {}

 private:
  bool WriteResource(const std::string& content, OutputResource* resource,
                     MessageHandler* handler);
  void OutlineStyle(HtmlElement* element, const std::string& content);
  void OutlineScript(HtmlElement* element, const std::string& content);

  // HTML strings interned into a symbol table.
  Atom s_link_;
  Atom s_script_;
  Atom s_style_;
  Atom s_rel_;
  Atom s_href_;
  Atom s_src_;
  Atom s_type_;
  // The style or script element we are in (if it hasn't been flushed).
  // If we are not in a script or style element, inline_element_ == NULL.
  HtmlElement* inline_element_;
  // Temporarily buffers the content between open and close of inline_element_.
  std::string buffer_;
  HtmlParse* html_parse_;
  ResourceManager* resource_manager_;
  bool outline_styles_;
  bool outline_scripts_;
};

}  // namespace net_instaweb

#endif  // NET_INSTAWEB_REWRITER_PUBLIC_OUTLINE_FILTER_H_
