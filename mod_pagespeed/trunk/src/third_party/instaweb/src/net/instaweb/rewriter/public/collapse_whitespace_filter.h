// Copyright 2010 and onwards Google Inc.
// Author: mdsteele@google.com (Matthew D. Steele)

#ifndef NET_INSTAWEB_REWRITER_PUBLIC_COLLAPSE_WHITESPACE_FILTER_H_
#define NET_INSTAWEB_REWRITER_PUBLIC_COLLAPSE_WHITESPACE_FILTER_H_

#include <vector>

#include "base/basictypes.h"
#include "net/instaweb/htmlparse/public/empty_html_filter.h"
#include "net/instaweb/util/public/atom.h"

namespace net_instaweb {

// Reduce the size of the HTML by collapsing whitespace (except within certain
// tags, e.g. <pre> and <script>).  Note that this is a dangerous filter, as
// CSS can be used to make the HTML whitespace-sensitive in unpredictable
// places; thus, it should only be used for content that you are sure will not
// do this.
//
// TODO(mdsteele): Use the CSS parser (once it's finished) to try to
// intelligently determine when the CSS "white-space: pre" property is in use;
// that would make this filter much safer.
class CollapseWhitespaceFilter : public EmptyHtmlFilter {
 public:
  explicit CollapseWhitespaceFilter(HtmlParse* html_parse);

  virtual void StartDocument();
  virtual void StartElement(HtmlElement* element);
  virtual void EndElement(HtmlElement* element);
  virtual void Characters(HtmlCharactersNode* characters);

 private:
  HtmlParse* html_parse_;
  std::vector<Atom> atom_stack_;
  AtomSet sensitive_tags_;

  DISALLOW_COPY_AND_ASSIGN(CollapseWhitespaceFilter);
};

}  // namespace net_instaweb

#endif  // NET_INSTAWEB_REWRITER_PUBLIC_COLLAPSE_WHITESPACE_FILTER_H_
