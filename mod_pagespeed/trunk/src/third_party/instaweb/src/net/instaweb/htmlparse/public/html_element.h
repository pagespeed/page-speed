// Copyright 2010 and onwards Google Inc.
// Author: jmarantz@google.com (Joshua Marantz)

#ifndef NET_INSTAWEB_HTMLPARSE_PUBLIC_HTML_ELEMENT_H_
#define NET_INSTAWEB_HTMLPARSE_PUBLIC_HTML_ELEMENT_H_

#include <string>
#include <vector>
#include "net/instaweb/htmlparse/public/html_parser_types.h"

namespace net_instaweb {

class HtmlElement {
 public:
  // Tags can be closed in three ways: implicitly (e.g. <img ..>),
  // briefly (e.g. <br/>), or explicitly (<a...>...</a>).  The
  // Lexer will always record the way it parsed a tag, but synthesized
  // elements will have AUTO_CLOSE, and rewritten elements may
  // no longer qualify for the closing style with which they were
  // parsed.
  enum CloseStyle {
    AUTO_CLOSE,      // synthesized tag, or not yet closed in source
    IMPLICIT_CLOSE,  // E.g. <img...> <meta...> <link...> <br...> <input...>
    EXPLICIT_CLOSE,  // E.g. <a href=...>anchor</a>
    BRIEF_CLOSE,     // E.g. <head/>
    UNCLOSED         // Was never closed in source
  };

  struct Attribute {
    // TODO(sligocki): check sanity of values (ex: no stray quotes).
    Attribute(const char* name, const char* value, const char* quote)
        : name_(name), value_(value), quote_(quote) {
    }
    const char* name_;
    const char* value_;
    const char* quote_;
  };

  ~HtmlElement();

  void AddAttribute(const Attribute& attribute);
  void AddAttribute(const char* name, const char* value, const char* quote) {
    AddAttribute(Attribute(name, value, quote));
  }

  // Small integer uniquely identifying the HTML element, primarily
  // for debugging.
  void set_sequence(int sequence) { sequence_ = sequence; }

  const char* tag() {return tag_;}
  int attribute_size() const {return attributes_.size(); }
  const Attribute& attribute(int i) const { return attributes_[i]; }
  Attribute& attribute(int i) { return attributes_[i]; }

  friend class HtmlParse;
  friend class HtmlLexer;

  CloseStyle close_style() const { return close_style_; }
  void set_close_style(CloseStyle style) { close_style_ = style; }

  // Render an element as a string for debugging.  This is not
  // intended as a fully legal serialization.
  void ToString(std::string* buf) const;
  void DebugPrint() const;

  int begin_line_number() const { return begin_line_number_; }
  int set_end_line_number() const { return end_line_number_; }

 private:
  // Begin/end event iterators are used by HtmlParse to keep track
  // of the span of events underneath an element.  This is primarily to
  // help delete the element.  Events are not public.
  void set_begin(const HtmlEventListIterator& begin) { begin_ = begin; }
  void set_end(const HtmlEventListIterator& end) { end_ = end; }
  HtmlEventListIterator begin() const { return begin_; }
  HtmlEventListIterator end() const { return end_; }

  void set_begin_line_number(int line) { begin_line_number_ = line; }
  void set_end_line_number(int line) { end_line_number_ = line; }

  // construct via HtmlParse::NewElement
  HtmlElement(const char* tag, const HtmlEventListIterator& begin,
      const HtmlEventListIterator& end);

  int sequence_;
  const char* tag_;
  std::vector<Attribute> attributes_;
  HtmlEventListIterator begin_;
  HtmlEventListIterator end_;
  CloseStyle close_style_;
  int begin_line_number_;
  int end_line_number_;
};
}

#endif  // NET_INSTAWEB_HTMLPARSE_PUBLIC_HTML_ELEMENT_H_
