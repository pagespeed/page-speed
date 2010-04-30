// Copyright 2010 Google Inc. All Rights Reserved.
// Author: jmaessen@google.com (Jan Maessen)

#ifndef NET_INSTAWEB_REWRITER_PUBLIC_IMG_REWRITE_FILTER_H_
#define NET_INSTAWEB_REWRITER_PUBLIC_IMG_REWRITE_FILTER_H_

#include "net/instaweb/rewriter/public/rewrite_filter.h"

#include "net/instaweb/htmlparse/public/html_element.h"
#include "net/instaweb/rewriter/public/input_resource.h"
#include "net/instaweb/util/public/atom.h"
#include <string>
#include "pagespeed/image_compression/png_optimizer.h"

namespace net_instaweb {

class ContentType;
class HtmlParse;
class ImgFilter;
class ResourceManager;

// Identify img tags in html.  For the moment, just log them.
// TODO(jmaessen): See which ones have immediately-obvious size info.
// TODO(jmaessen): Rewrite resource urls
// TODO(jmaessen): Provide alternate resources at rewritten urls somehow.
// TODO(jmaessen): Run image optimization on alternate resources where useful.
// TODO(jmaessen): Big open question: how best to link pulled-in resources to
//     rewritten urls, when in general those urls will be in a different domain.
class ImgRewriteFilter : public RewriteFilter {
 public:
  ImgRewriteFilter(StringPiece path_prefix,
                   HtmlParse* html_parse,
                   ResourceManager* resource_manager);
  virtual void EndElement(HtmlElement* element);
  virtual void Flush();
  virtual bool Fetch(StringPiece resource, Writer* writer,
                     const MetaData& request_header,
                     MetaData* response_headers,
                     UrlAsyncFetcher* fetcher,
                     MessageHandler* message_handler,
                     UrlAsyncFetcher::Callback* callback);

 private:
  // These are just helper methods.
  void WriteBytesWithExtension(const ContentType& content_type,
                               const std::string& contents,
                               HtmlElement::Attribute* src);
  void OptimizePng(pagespeed::image_compression::PngReaderInterface* reader,
                   HtmlElement::Attribute* src, InputResource* img_resource);
  void OptimizeJpeg(HtmlElement::Attribute* src, InputResource* img_resource);
  void OptimizeImgResource(HtmlElement::Attribute* src,
                           InputResource* img_resource);

  HtmlParse* html_parse_;
  ImgFilter* img_filter_;
  ResourceManager* resource_manager_;
  const Atom s_width_;
  const Atom s_height_;
};

}  // namespace net_instaweb

#endif  // NET_INSTAWEB_REWRITER_PUBLIC_IMG_REWRITE_FILTER_H_
