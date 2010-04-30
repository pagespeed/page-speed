// Copyright 2010 Google Inc. All Rights Reserved.
// Author: jmaessen@google.com (Jan Maessen)

#ifndef NET_INSTAWEB_REWRITER_PUBLIC_IMG_FILTER_H_
#define NET_INSTAWEB_REWRITER_PUBLIC_IMG_FILTER_H_

#include "net/instaweb/htmlparse/public/html_element.h"
#include "net/instaweb/util/public/atom.h"

namespace net_instaweb {
class HtmlParse;

class ImgFilter {
 public:
  explicit ImgFilter(HtmlParse* html_parse);

  // Examine HTML element and determine if it is an img with a src.  If so
  // extract the src attribute and return it, otherwise return NULL.
  HtmlElement::Attribute* ParseImgElement(HtmlElement* element) {
    if (element->tag() == s_img_) {
      return element->FindAttribute(s_src_);
    }
    return NULL;
  }

 private:
  const Atom s_img_;
  const Atom s_src_;
};

}  // namespace net_instaweb

#endif  // NET_INSTAWEB_REWRITER_PUBLIC_IMG_FILTER_H_
