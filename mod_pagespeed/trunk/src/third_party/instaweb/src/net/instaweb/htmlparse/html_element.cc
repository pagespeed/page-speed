// Copyright 2010 and onwards Google Inc.
// Author: jmarantz@google.com (Joshua Marantz)

#include "public/html_element.h"
#include <stdio.h>

namespace net_instaweb {

HtmlElement::HtmlElement(const char* tag,
    const HtmlEventListIterator& begin, const HtmlEventListIterator& end)
    : sequence_(-1),
      tag_(tag),
      begin_(begin),
      end_(end),
      close_style_(AUTO_CLOSE),
      begin_line_number_(-1),
      end_line_number_(-1) {
}

HtmlElement::~HtmlElement() {
}

void HtmlElement::AddAttribute(const Attribute& attribute) {
  attributes_.push_back(attribute);
}

void HtmlElement::ToString(std::string* buf) const {
  *buf += "<";
  *buf += tag_;
  for (size_t i = 0; i < attributes_.size(); ++i) {
    const Attribute& attribute = attributes_[i];
    *buf += ' ';
    *buf += attribute.name_;
    if (attribute.value_ != NULL) {
      *buf += "=";
      const char* quote = (attribute.quote_ != NULL) ? attribute.quote_ : "?";
      *buf += quote;
      *buf += attribute.value_;
      *buf += quote;
    }
  }
  switch (close_style_) {
    case AUTO_CLOSE:       *buf += "?>"; break;
    case IMPLICIT_CLOSE:   *buf += ">";  break;
    case EXPLICIT_CLOSE:   *buf += "></"; *buf += tag_; *buf += ">"; break;
    case BRIEF_CLOSE:      *buf += "/>"; break;
  }
  if ((begin_line_number_ != -1) || (end_line_number_ != -1)) {
    *buf += " ";
    char ibuf[100];
    if (begin_line_number_ != -1) {
      snprintf(ibuf, sizeof(ibuf), "%d", begin_line_number_);
      *buf += ibuf;
    }
    *buf += "...";
    if (end_line_number_ != -1) {
      snprintf(ibuf, sizeof(ibuf), "%d", end_line_number_);
      *buf += ibuf;
    }
  }
}

void HtmlElement::DebugPrint() const {
  std::string buf;
  ToString(&buf);
  fprintf(stdout, "%s\n", buf.c_str());
}
}
