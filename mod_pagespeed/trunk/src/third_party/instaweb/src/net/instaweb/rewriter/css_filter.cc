// Copyright 2010 and onwards Google Inc.
// Author: jmarantz@google.com (Joshua Marantz)

#include "net/instaweb/rewriter/public/css_filter.h"

#include <assert.h>
#include "net/instaweb/htmlparse/public/html_parse.h"
#include "net/instaweb/htmlparse/public/html_element.h"
#include "net/instaweb/util/public/google_url.h"
#include "net/instaweb/util/public/message_handler.h"
#include <string>
#include "net/instaweb/util/public/writer.h"

namespace {
const char kStylesheet[] = "stylesheet";
const char kTextCss[] = "text/css";
}

namespace net_instaweb {

// Finds CSS files and calls another filter.
CssFilter::CssFilter(HtmlParse* html_parse) {
  s_link_  = html_parse->Intern("link");
  s_href_  = html_parse->Intern("href");
  s_type_  = html_parse->Intern("type");
  s_rel_   = html_parse->Intern("rel");
  s_media_ = html_parse->Intern("media");
}

// TODO(jmarantz): add test for this method to css_filter_test.cc
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

namespace {

bool ExtractQuote(StringPiece* url, char* quote) {
  bool ret = false;
  int size = url->size();
  if (size > 2) {
    *quote = (*url)[0];
    if (((*quote == '\'') || (*quote == '"')) && (*quote == (*url)[size - 1])) {
      ret = true;
      *url = url->substr(1, size - 2);
    }
  }
  return ret;
}

}  // namespace

// TODO(jmarantz): replace this scan-and-replace-in-one-shot methdology with
// a proper scanner/parser/filtering mechanism akin to HtmlParse/HtmlLexer.
// See http://www.w3.org/Style/CSS/SAC/ for the C Parser.
bool CssFilter::AbsolutifyUrls(
    const StringPiece& contents, const std::string& base_url,
    Writer* writer, MessageHandler* handler) {
  size_t pos = 0;
  size_t prev_pos = 0;
  bool ok = true;

  // If the CSS url was specified with an absolute path, use that to
  // absolutify any URLs referenced in the CSS text.
  GURL base_gurl(base_url);
  if (base_gurl.is_valid()) {
    GURL base_host = base_gurl.GetWithEmptyPath();
    if (base_host.is_valid()) {
      while (ok && ((pos = contents.find("url(", pos)) != StringPiece::npos)) {
        ok = writer->Write(contents.substr(prev_pos, pos - prev_pos), handler);
        prev_pos = pos;
        pos += 4;
        size_t end_of_url = contents.find(')', pos);
        if ((end_of_url != StringPiece::npos) && (end_of_url != pos)) {
          StringPiece url = contents.substr(pos, end_of_url - pos);
          char quote;
          bool is_quoted = ExtractQuote(&url, &quote);
          std::string url_string(url.data(), url.size());
          GURL gurl(url_string);

          // Relative paths are considered invalid by GURL, and those are the
          // ones we need to resolve.
          if (!gurl.is_valid()) {
            GURL resolved = base_host.Resolve(url_string.c_str());
            if (resolved.is_valid()) {
              ok = writer->Write("url(", handler);
              if (is_quoted) {
                writer->Write(StringPiece(&quote, 1), handler);
              }
              ok = writer->Write(resolved.spec().c_str(), handler);
              if (is_quoted) {
                writer->Write(StringPiece(&quote, 1), handler);
              }
              ok = writer->Write(")", handler);
              prev_pos = end_of_url + 1;
            } else {
              int line = 1;
              for (size_t i = 0; i < pos; ++i) {
                line += (contents[i] == '\n');
              }
              handler->Error(
                  base_url.c_str(), line,
                  "CSS URL resolution failed: %s", url_string.c_str());
            }
          }
        }
      }
    }
  }
  if (ok) {
    ok = writer->Write(contents.substr(prev_pos), handler);
  }
  return ok;
}

}  // namespace net_instaweb
