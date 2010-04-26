// Copyright 2010 and onwards Google Inc.
// Author: jmarantz@google.com (Joshua Marantz)

#include "public/html_element.h"
#include <stdio.h>
#include <string>

namespace net_instaweb {

HtmlElement::HtmlElement(Atom tag,
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
  for (int i = 0, n = attribute_size(); i < n; ++i) {
    delete attributes_[i];
  }
}

const HtmlElement::Attribute* HtmlElement::FirstAttributeWithName(
    const Atom name) const {
  for (int i = 0; i < attribute_size(); ++i) {
    const Attribute* attribute = attributes_[i];
    if (attribute->name() == name) {
      return attribute;
    }
  }
  return NULL;
}

void HtmlElement::ToString(std::string* buf) const {
  *buf += "<";
  *buf += tag_.c_str();
  for (int i = 0; i < attribute_size(); ++i) {
    const Attribute& attribute = *attributes_[i];
    *buf += ' ';
    *buf += attribute.name().c_str();
    if (attribute.value() != NULL) {
      *buf += "=";
      const char* quote = (attribute.quote() != NULL) ? attribute.quote() : "?";
      *buf += quote;
      *buf += attribute.value();
      *buf += quote;
    }
  }
  switch (close_style_) {
    case AUTO_CLOSE:       *buf += "> (not yet closed)"; break;
    case IMPLICIT_CLOSE:   *buf += ">";  break;
    case EXPLICIT_CLOSE:   *buf += "></";
                           *buf += tag_.c_str();
                           *buf += ">";
                           break;
    case BRIEF_CLOSE:      *buf += "/>"; break;
    case UNCLOSED:         *buf += "> (unclosed)"; break;
  }
  if ((begin_line_number_ != -1) || (end_line_number_ != -1)) {
    *buf += " ";
    if (begin_line_number_ != -1) {
      *buf += IntegerToString(begin_line_number_);
    }
    *buf += "...";
    if (end_line_number_ != -1) {
      *buf += IntegerToString(end_line_number_);
    }
  }
}

void HtmlElement::DebugPrint() const {
  std::string buf;
  ToString(&buf);
  fprintf(stdout, "%s\n", buf.c_str());
}


bool HtmlElement::ReplaceAttribute(Atom name, const char* value) {
  bool ret = false;
  Attribute* attribute = FirstAttributeWithName(name);
  if (attribute != NULL) {
    attribute->set_value(value);
    ret = true;
  }
  return ret;
}

}  // namespace net_instaweb
