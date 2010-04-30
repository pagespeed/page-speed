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

}  // namespace net_instaweb
