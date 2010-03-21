// Copyright 2010 and onwards Google Inc.
// Author: jmarantz@google.com (Joshua Marantz)

#include "public/html_element.h"

namespace net_instaweb {

HtmlElement::HtmlElement(const char* tag,
    const HtmlEventListIterator& begin, const HtmlEventListIterator& end)
    : sequence_(-1),
      tag_(tag),
      begin_(begin),
      end_(end),
      close_style_(AUTO_CLOSE) {
}

HtmlElement::~HtmlElement() {
}

void HtmlElement::AddAttribute(const Attribute& attribute) {
  attributes_.push_back(attribute);
}
}
