// Copyright 2010 and onwards Google Inc.
// Author: jmarantz@google.com (Joshua Marantz)

#include "net/instaweb/rewriter/public/css_filter.h"

#include <assert.h>
#include "net/instaweb/htmlparse/public/html_parse.h"
#include "net/instaweb/htmlparse/public/html_element.h"
#include "net/instaweb/util/public/message_handler.h"
#include <string>

namespace {
const char kStylesheet[] = "stylesheet";
const char kTextCss[] = "text/css";
}

namespace net_instaweb {

// Finds CSS files and calls another filter.
CssFilter::CssFilter(HtmlParse* html_parse) {
  s_link_ =       html_parse->Intern("link");
  s_href_ =       html_parse->Intern("href");
  s_type_ =       html_parse->Intern("type");
  s_rel_ =        html_parse->Intern("rel");
  s_media_  =     html_parse->Intern("media");
}

bool CssFilter::ParseCssElement(
    HtmlElement* element, HtmlElement::Attribute** href, const char** media) {
  bool ret = false;
  *media = "";
  *href = NULL;
  if (element->tag() == s_link_) {
    // We must have all attributes rel='stylesheet' href='name.css', and
    // type='text/css', although they can be in any order.  If there are,
    // other attributes, we better learn about them so we don't lose them
    // in css_combine_filter.cc.
    int num_attrs = element->attribute_size();

    // 'media=' is optional, but our filter requires href=*, and rel=stylesheet,
    // and type=text/css.
    //
    // TODO(jmarantz): Consider recognizing a wider variety of CSS references,
    // including inline css so that the outline_filter can use it.
    if ((num_attrs == 3) || (num_attrs == 4)) {
      for (int i = 0; i < num_attrs; ++i) {
        HtmlElement::Attribute& attr = element->attribute(i);
        if (attr.name() == s_href_) {
          *href = &attr;
          ret = true;
        } else if (attr.name() == s_media_) {
          *media = attr.value();
        } else if (!(((attr.name() == s_rel_) &&
                      (strcasecmp(attr.value(), kStylesheet) == 0)) ||
                     ((attr.name() == s_type_) &&
                      (strcasecmp(attr.value(), kTextCss) == 0)))) {
          // TODO(jmarantz): warn when CSS elements aren't quite what we expect?
          ret = false;
          break;
        }
      }
    }
  }
  return ret;
}

}  // namespace net_instaweb
