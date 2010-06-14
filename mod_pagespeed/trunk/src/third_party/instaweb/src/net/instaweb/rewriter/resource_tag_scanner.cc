// Copyright 2010 and onwards Google Inc.
// Author: jmarantz@google.com (Joshua Marantz)

#include "net/instaweb/rewriter/public/resource_tag_scanner.h"

namespace net_instaweb {

// Finds resource references.
ResourceTagScanner::ResourceTagScanner(HtmlParse* html_parse)
    : css_tag_scanner_(html_parse),
      img_tag_scanner_(html_parse),
      script_tag_scanner_(html_parse) {
}

HtmlElement::Attribute* ResourceTagScanner::ScanElement(HtmlElement* element) {
  HtmlElement::Attribute* attr = NULL;
  const char* media;
  if (!css_tag_scanner_.ParseCssElement(element, &attr, &media)) {
    attr = img_tag_scanner_.ParseImgElement(element);
    if (attr == NULL) {
      attr = script_tag_scanner_.ParseScriptElement(element);
    }
  }
  return attr;
}

}  // namespace net_instaweb
