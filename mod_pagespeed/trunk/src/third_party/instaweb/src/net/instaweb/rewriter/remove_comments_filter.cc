// Copyright 2010 and onwards Google Inc.
// Author: mdsteele@google.com (Matthew D. Steele)

#include "net/instaweb/rewriter/public/remove_comments_filter.h"

#include "net/instaweb/htmlparse/public/html_parse.h"

namespace net_instaweb {

RemoveCommentsFilter::RemoveCommentsFilter(HtmlParse* html_parse)
    : html_parse_(html_parse) {}

void RemoveCommentsFilter::Comment(HtmlCommentNode* comment) {
  html_parse_->DeleteElement(comment);
}

}  // namespace net_instaweb
