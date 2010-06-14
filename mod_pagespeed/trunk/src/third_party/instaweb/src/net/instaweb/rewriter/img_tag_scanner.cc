// Copyright 2010 Google Inc. All Rights Reserved.
// Author: jmaessen@google.com (Jan Maessen)

#include "net/instaweb/rewriter/public/img_tag_scanner.h"

#include "net/instaweb/htmlparse/public/html_element.h"
#include "net/instaweb/htmlparse/public/html_parse.h"
#include "net/instaweb/util/public/atom.h"

namespace net_instaweb {

ImgTagScanner::ImgTagScanner(HtmlParse* html_parse)
    : s_img_(html_parse->Intern("img")),
      s_src_(html_parse->Intern("src")) {
}

}  // namespace net_instaweb
