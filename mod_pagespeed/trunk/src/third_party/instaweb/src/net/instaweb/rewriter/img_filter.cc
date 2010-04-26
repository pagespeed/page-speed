// Copyright 2010 Google Inc. All Rights Reserved.
// Author: jmaessen@google.com (Jan Maessen)

#include "net/instaweb/rewriter/public/img_filter.h"

#include "net/instaweb/htmlparse/public/html_element.h"
#include "net/instaweb/htmlparse/public/html_parse.h"
#include "net/instaweb/util/public/atom.h"

namespace net_instaweb {

ImgFilter::ImgFilter(HtmlParse* html_parse)
    : s_img_(html_parse->Intern("img")),
      s_src_(html_parse->Intern("src")) {
}

const char* ImgFilter::ParseImgElement(const HtmlElement* element) {
  const char* src = NULL;
  if (element->tag() == s_img_) {
    const HtmlElement::Attribute* src_attr =
        element->FirstAttributeWithName(s_src_);
    if (src_attr != NULL) {
      src = src_attr->value();
    }
  }
  return src;
}

bool ImgFilter::ReplaceSrc(const char *new_src, HtmlElement* element) {
  bool res = false;
  if (element->tag() == s_img_) {
    res = element->ReplaceAttribute(s_src_, new_src);
  }
  return res;
}

}  // namespace net_instaweb
