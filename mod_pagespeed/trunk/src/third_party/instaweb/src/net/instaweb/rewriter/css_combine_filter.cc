// Copyright 2010 and onwards Google Inc.
// Author: jmarantz@google.com (Joshua Marantz)

#include "net/instaweb/rewriter/public/css_combine_filter.h"

#include <assert.h>
#include "net/instaweb/rewriter/public/input_resource.h"
#include "net/instaweb/rewriter/public/output_resource.h"
#include "net/instaweb/rewriter/public/resource_manager.h"
#include "net/instaweb/htmlparse/public/html_parse.h"
#include "net/instaweb/htmlparse/public/html_element.h"
#include "net/instaweb/util/public/content_type.h"
#include "net/instaweb/util/public/message_handler.h"
#include "net/instaweb/util/public/simple_meta_data.h"
#include <string>
#include "net/instaweb/util/public/string_writer.h"

namespace net_instaweb {

// TODO(jmarantz) We exhibit zero intelligence about which css files to
// combine; we combine whatever is possible.  This can reduce performance
// by combining highly cacheable shared resources with transient ones.
//
// TODO(jmarantz): We do not recognize IE directives as spriting boundaries.
// We should supply a meaningful IEDirective method as a boundary.
//
// TODO(jmarantz): allow spriting of CSS elements found in the body, whether
// or not the head has already been flushed.

CssCombineFilter::CssCombineFilter(const char* path_prefix,
                                   HtmlParse* html_parse,
                                   ResourceManager* resource_manager)
    : RewriteFilter(path_prefix),
      html_parse_(html_parse),
      resource_manager_(resource_manager),
      css_filter_(html_parse) {
  s_head_ = html_parse->Intern("head");
  s_link_ = html_parse->Intern("link");
  s_href_ = html_parse->Intern("href");
  s_type_ = html_parse->Intern("type");
  s_rel_  = html_parse->Intern("rel");
  head_element_ = NULL;
}

void CssCombineFilter::StartDocument() {
  head_element_ = NULL;
}

void CssCombineFilter::StartElement(HtmlElement* element) {
  if (element->tag() == s_head_) {
    head_element_ = element;
  }
}

void CssCombineFilter::EndElement(HtmlElement* element) {
  const char* href;
  const char* media;
  if (css_filter_.ParseCssElement(element, &href, &media)) {
    css_elements_.push_back(element);
  } else if (element->tag() == s_head_) {
    EmitCombinations();
  }
}

// An IE directive that includes any stylesheet info should be a barrier
// for css spriting.  It's OK to emit the spriting we've seen so far.
void CssCombineFilter::IEDirective(const std::string& directive) {
  // TODO(jmarantz): consider recursively invoking the parser and
  // parsing all the IE-specific code properly.
  if (directive.find("stylesheet") != std::string::npos) {
  }
}

void CssCombineFilter::Flush() {
  // TODO(jmarantz): Ideally, all the css links will be encountered in the
  // <head>, before the first flush.  It's possible we'll get a Flush,
  // during the <head> parse, and there may be some css files before it,
  // and some afterward.  And there may be css links encountered in the body,
  // and there may have Flushed our head css combinations first.  So all of that
  // will have to be dealt with by calling EmitCombinations, after finding the
  // appropriate place in the DOM to insert the combination.
  //
  // The best performance will come when the entire document is parsed
  // without a Flush, in which case we can move all the css links into
  // the <head>, but even that is not yet implemented.
  css_elements_.clear();
}

void CssCombineFilter::EmitCombinations() {
  MessageHandler* message_handler = html_parse_->message_handler();

  // It's possible that we'll have found 2 css files to combine, but one
  // of them became non-rewritable due to a flush, and thus we'll wind
  // up spriting just one file, so do a first pass counting rewritable
  // css linkes.  Also, load the CSS content in this pass.  We will only
  // do a combine if we have more than one css element that successfully
  // loaded.
  std::vector<HtmlElement*> combine_elements;
  std::vector<InputResource*> combine_resources;
  for (int i = 0, n = css_elements_.size(); i < n; ++i) {
    HtmlElement* element = css_elements_[i];
    const char* media;
    const char* href;
    if (css_filter_.ParseCssElement(element, &href, &media) &&
        html_parse_->IsRewritable(element)) {
      // TODO(jmarantz): consider async loads; exclude css file
      // from the combination that are not yet loaded.  For now, our
      // loads are blocking.  Need to understand Apache module
      InputResource* css_resource =
          resource_manager_->CreateInputResource(href);
      if ((css_resource != NULL) && css_resource->Read(message_handler)) {
        if (*media != '\0') {
          // TODO(jmarantz): Annotate combined sections with 'media' as needed
          // css_resource->AddAttribute("media", media.c_str());
        }
        combine_resources.push_back(css_resource);

        // Try to add this resource to the combination.  We are not yet
        // committed to the combination because we haven't written the
        // contents to disk yet, so don't mutate the DOM but keep
        // track of which elements will be involved
        combine_elements.push_back(element);
      }
    }
  }

  if (combine_elements.size() > 1) {
    // Ideally like to have a data-driven service tell us which elements should
    // be combined together.  Note that both the resources and the elements
    // are managed, so we don't delete them even if the spriting fails.
    OutputResource* combination =
        resource_manager_->CreateOutputResource(kContentTypeCss);
    HtmlElement* combine_element = html_parse_->NewElement(s_link_);
    combine_element->AddAttribute(s_rel_, "stylesheet", "\"");
    combine_element->AddAttribute(s_type_, "text/css", "\"");

    // Start building up the combination.  At this point we are still
    // not committed to the combination, because the 'write' can fail.
    //
    // TODO(jmarantz): determinee if combination is already written.
    bool written = combination->StartWrite(message_handler);
    for (int i = 0, n = combine_resources.size(); written && (i < n); ++i) {
      const std::string& contents = combine_resources[i]->contents();
      written = combination->WriteChunk(contents.data(), contents.size(),
                                        message_handler);
    }
    if (written) {
      written = combination->EndWrite(message_handler);
    }

    // We've collected at least two CSS files to combine, and whose
    // HTML elements are in the current flush window.  Last step
    // is to write the combination.
    if (written && combination->IsReadable()) {
      // commit by removing the elements from the DOM.
      for (size_t i = 0; i < combine_elements.size(); ++i) {
        html_parse_->DeleteElement(combine_elements[i]);
      }
      combine_element->AddAttribute(s_href_, combination->url().c_str(), "\"");
      html_parse_->InsertElementBeforeCurrent(combine_element);
    }
  }
  css_elements_.clear();
}

bool CssCombineFilter::Fetch(StringPiece resource, Writer* writer,
                             const MetaData& request_header,
                             MetaData* response_headers,
                             UrlAsyncFetcher* fetcher,
                             MessageHandler* message_handler,
                             UrlAsyncFetcher::Callback* callback) {
  return false;
}

}  // namespace net_instaweb
