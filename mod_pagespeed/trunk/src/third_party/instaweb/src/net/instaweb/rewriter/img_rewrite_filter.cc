// Copyright 2010 Google Inc. All Rights Reserved.
// Author: jmaessen@google.com (Jan Maessen)

#include "net/instaweb/rewriter/public/img_rewrite_filter.h"

#include <string>
#include "net/instaweb/htmlparse/public/html_element.h"
#include "net/instaweb/htmlparse/public/html_parse.h"
#include "net/instaweb/rewriter/public/resource_manager.h"

namespace net_instaweb {

ImgRewriteFilter::ImgRewriteFilter(HtmlParse* html_parse,
                                   ResourceManager* resource_manager)
    : html_parse_(html_parse),
      resource_manager_(resource_manager) {
  s_img_ = html_parse->Intern("img");
}

void ImgRewriteFilter::EndElement(HtmlElement* element) {
  if (element->tag() == s_img_) {
    std::string tagstring;
    element->ToString(&tagstring);
    html_parse_->Info(
        html_parse_->filename(), element->begin_line_number(),
        "Found image: %s", tagstring.c_str());
  }
}

void ImgRewriteFilter::Flush() {
  // TODO(jmaessen): wait here for resources to have been rewritten??
}
}
