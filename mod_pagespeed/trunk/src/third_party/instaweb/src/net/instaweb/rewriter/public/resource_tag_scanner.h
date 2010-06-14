// Copyright 2010 and onwards Google Inc.
// Author: jmarantz@google.com (Joshua Marantz)

#ifndef NET_INSTAWEB_REWRITER_PUBLIC_RESOURCE_TAG_SCANNER_H_
#define NET_INSTAWEB_REWRITER_PUBLIC_RESOURCE_TAG_SCANNER_H_

#include "net/instaweb/rewriter/public/css_tag_scanner.h"
#include "net/instaweb/rewriter/public/img_tag_scanner.h"
#include "net/instaweb/rewriter/public/script_tag_scanner.h"

namespace net_instaweb {

class ResourceTagScanner {
 public:
  explicit ResourceTagScanner(HtmlParse* html_parse);

  // Examines an HTML element to determine if it's a link to any sort
  // of resource, extracting out the HREF or SRC.
  HtmlElement::Attribute* ScanElement(HtmlElement* element);

 private:
  CssTagScanner css_tag_scanner_;
  ImgTagScanner img_tag_scanner_;
  ScriptTagScanner script_tag_scanner_;
};

}  // namespace net_instaweb

#endif  // NET_INSTAWEB_REWRITER_PUBLIC_RESOURCE_TAG_SCANNER_H_
