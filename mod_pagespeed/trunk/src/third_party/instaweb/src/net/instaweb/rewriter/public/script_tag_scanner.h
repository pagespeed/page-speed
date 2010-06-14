// Copyright 2010 Google Inc. All Rights Reserved.
// Author: jmarantz@google.com (Joshua Marantz)

#ifndef NET_INSTAWEB_REWRITER_PUBLIC_SCRIPT_TAG_SCANNER_H_
#define NET_INSTAWEB_REWRITER_PUBLIC_SCRIPT_TAG_SCANNER_H_

#include "net/instaweb/htmlparse/public/html_element.h"
#include "net/instaweb/util/public/atom.h"

namespace net_instaweb {
class HtmlParse;

class ScriptTagScanner {
 public:
  explicit ScriptTagScanner(HtmlParse* html_parse);

  // Examine HTML element and determine if it is an script with a src.  If so,
  // extract the src attribute and return it, otherwise return NULL.
  HtmlElement::Attribute* ParseScriptElement(HtmlElement* element) {
    if (element->tag() == s_script_) {
      return element->FindAttribute(s_src_);
    }
    return NULL;
  }

 private:
  const Atom s_script_;
  const Atom s_src_;
};

}  // namespace net_instaweb

#endif  // NET_INSTAWEB_REWRITER_PUBLIC_SCRIPT_TAG_SCANNER_H_
