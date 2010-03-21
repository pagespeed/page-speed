// Copyright 2010 and onwards Google Inc.
// Author: jmarantz@google.com (Joshua Marantz)

#include "public/base_tag_filter.h"

#include <string>
#include "public/add_head_filter.h"
#include "net/instaweb/htmlparse/public/html_parse.h"
#include "net/instaweb/htmlparse/public/html_element.h"

namespace net_instaweb {
BaseTagFilter::BaseTagFilter(HtmlParse* html_parse) {
  s_head_ = html_parse->Intern("head");
  s_base_ = html_parse->Intern("base");
  s_href_ = html_parse->Intern("href");
  s_head_element_ = NULL;
  found_base_tag_ = false;
  html_parse_ = html_parse;
}

void BaseTagFilter::StartDocument() {
  s_head_element_ = NULL;
  found_base_tag_ = false;
}

void BaseTagFilter::StartElement(HtmlElement* element) {
  if (element->tag() == s_head_) {
    s_head_element_ = element;
  } else if (s_head_element_ != NULL) {
    if (element->tag() == s_base_) {
      // There is already a base tag.  See if it's specified an href.
      for (int i = 0; i < element->attribute_size(); ++i) {
        HtmlElement::Attribute& attribute = element->attribute(i);
        if (attribute.name_ == s_href_) {
          // For now let's assume that the explicit base-tag in
          // the source should left alone if it has an href.
          // attribute.second = url_.c_str();
          found_base_tag_ = true;
          break;
        }
      }
    }
  }
}

void BaseTagFilter::EndElement(HtmlElement* element) {
  if (element == s_head_element_) {
    s_head_element_ = NULL;
    if (!found_base_tag_) {
      found_base_tag_ = true;
      std::vector<std::string> head_atts;
      HtmlElement* element = html_parse_->NewElement(s_base_);
      element->AddAttribute(s_href_, url_.c_str(), "\"");
      html_parse_->InsertElementBeforeCurrent(element);
      found_base_tag_ = true;
    }
  }
}
}
