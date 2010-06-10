// Copyright 2010 and onwards Google Inc.
// Author: mdsteele@google.com (Matthew D. Steele)

#ifndef NET_INSTAWEB_REWRITER_PUBLIC_REMOVE_COMMENTS_FILTER_H_
#define NET_INSTAWEB_REWRITER_PUBLIC_REMOVE_COMMENTS_FILTER_H_

#include "base/basictypes.h"
#include "net/instaweb/htmlparse/public/empty_html_filter.h"

namespace net_instaweb {

// Reduce the size of the HTML by removing all HTML comments (except those
// which are IE directives).  Note that this is a potentially dangerous
// optimization; if a site is using comments for some squirrelly purpose, then
// removing those comments might break something.
class RemoveCommentsFilter : public EmptyHtmlFilter {
 public:
  explicit RemoveCommentsFilter(HtmlParse* html_parse);

  virtual void Comment(HtmlCommentNode* comment);

 private:
  HtmlParse* html_parse_;

  DISALLOW_COPY_AND_ASSIGN(RemoveCommentsFilter);
};

}  // namespace net_instaweb

#endif  // NET_INSTAWEB_REWRITER_PUBLIC_REMOVE_COMMENTS_FILTER_H_
