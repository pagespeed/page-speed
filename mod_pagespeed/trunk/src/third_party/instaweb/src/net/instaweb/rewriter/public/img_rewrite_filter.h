// Copyright 2010 Google Inc. All Rights Reserved.
// Author: jmaessen@google.com (Jan Maessen)

#ifndef NET_INSTAWEB_REWRITER_PUBLIC_IMG_REWRITE_FILTER_H_
#define NET_INSTAWEB_REWRITER_PUBLIC_IMG_REWRITE_FILTER_H_

#include <string>
#include "net/instaweb/htmlparse/public/empty_html_filter.h"

namespace net_instaweb {
class HtmlParse;
class ResourceManager;

// Identify img tags in html.  For the moment, just log them.
// TODO(jmaessen): See which ones have immediately-obvious size info.
// TODO(jmaessen): Rewrite resource urls
// TODO(jmaessen): Provide alternate resources at rewritten urls somehow.
// TODO(jmaessen): Run image optimization on alternate resources where useful.
// TODO(jmaessen): Big open question: how best to link pulled-in resources to
//     rewritten urls, when in general those urls will be in a different domain.
class ImgRewriteFilter : public EmptyHtmlFilter {
 public:
  ImgRewriteFilter(HtmlParse* html_parse, ResourceManager* resource_manager);
  virtual void EndElement(HtmlElement* element);
  virtual void Flush();

 private:
  const char* s_img_;
  HtmlParse* html_parse_;
  ResourceManager* resource_manager_;
};
}

#endif  // NET_INSTAWEB_REWRITER_PUBLIC_IMG_REWRITE_FILTER_H_
