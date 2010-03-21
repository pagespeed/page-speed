// Copyright 2010 and onwards Google Inc.
// Author: jmarantz@google.com (Joshua Marantz)

#ifndef NET_INSTAWEB_REWRITER_PUBLIC_CSS_SPRITE_FILTER_H_
#define NET_INSTAWEB_REWRITER_PUBLIC_CSS_SPRITE_FILTER_H_

#include <string>
#include <vector>

#include "net/instaweb/htmlparse/public/empty_html_filter.h"

namespace net_instaweb {
class ResourceManager;

class CssSpriteFilter : public EmptyHtmlFilter {
 public:
  CssSpriteFilter(HtmlParse* html_parse, ResourceManager* resource_manager);

  virtual void StartDocument();
  virtual void StartElement(HtmlElement* element);
  virtual void EndElement(HtmlElement* element);
  virtual void Flush();
  virtual void IEDirective(const std::string& directive);

 private:
  const char* FindCssHref(
      const HtmlElement*, std::string* media);
  void EmitSprites();

  const char* s_head_;
  const char* s_type_;
  const char* s_link_;
  const char* s_href_;
  const char* s_text_css_;
  const char* s_rel_;
  const char* s_media_;
  const char* s_stylesheet_;
  bool found_css_sprite_;
  std::vector<HtmlElement*> css_elements_;
  HtmlParse* html_parse_;
  HtmlElement* head_element_;  // Pointer to head element for future use
  std::vector<HtmlElement*> element_stack_;
  ResourceManager* resource_manager_;
};
}

#endif  // NET_INSTAWEB_REWRITER_PUBLIC_CSS_SPRITE_FILTER_H_
