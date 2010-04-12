// Copyright 2010 and onwards Google Inc.
// Author: jmarantz@google.com (Joshua Marantz)

#include "public/add_head_filter.h"

#include <assert.h>
#include "net/instaweb/htmlparse/public/html_parse.h"
#include "net/instaweb/htmlparse/public/html_element.h"

namespace net_instaweb {
AddHeadFilter::AddHeadFilter(HtmlParse* html_parse)
    : html_parse_(html_parse) {
  found_head_ = false;
  s_head_ = html_parse->Intern("head");
  s_body_ = html_parse->Intern("body");
}

void AddHeadFilter::StartDocument() {
  found_head_ = false;
}

void AddHeadFilter::StartElement(HtmlElement* element) {
  if (!found_head_) {
    if (element->tag() == s_body_) {
      HtmlElement* head_element = html_parse_->NewElement(s_head_);
      html_parse_->InsertElementBeforeCurrent(head_element);
      found_head_ = true;
    } else if (element->tag() == s_head_) {
      found_head_ = true;
    }
  }
}

void AddHeadFilter::EndDocument() {
  if (!found_head_) {
    // In order to insert a <head> in a docucument that lacks one, we
    // must first find the body.  If we get through the whole doc without
    // finding a <head> or a <body> then this filter will have failed to
    // add a head.
    html_parse_->ErrorHere("Reached end of document without finding <body>");
  }
}
}
