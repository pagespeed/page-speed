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

  // Examine HTML element and determine if it is an img with a src.
  // If so extract the value of the src attribute and return it,
  // otherwise return NULL.
  // NOTE: the returned value continues to be owned by element.
  // This means it goes away if we modify the element value in future.
  // TODO(jmaessen): Replace char * with StringPiece or equivalent to make
  // ownership issues clear.
  const char* ParseImgElement(const HtmlElement* element);

  // Examine HTML element, and if it is an img replace its src with
  // new_src (taking ownership of the char *).
  bool ReplaceSrc(const char *new_src, HtmlElement* element);

 private:
  const Atom s_img_;
  const Atom s_src_;
};

}  // namespace net_instaweb

#endif  // NET_INSTAWEB_REWRITER_PUBLIC_IMG_FILTER_H_
