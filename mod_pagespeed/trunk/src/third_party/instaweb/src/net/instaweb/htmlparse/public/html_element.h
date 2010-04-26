// Copyright 2010 and onwards Google Inc.
// Author: jmarantz@google.com (Joshua Marantz)

#ifndef NET_INSTAWEB_HTMLPARSE_PUBLIC_HTML_ELEMENT_H_
#define NET_INSTAWEB_HTMLPARSE_PUBLIC_HTML_ELEMENT_H_

#include <vector>
#include "base/scoped_ptr.h"
#include "net/instaweb/htmlparse/public/html_parser_types.h"
#include "net/instaweb/util/public/atom.h"
#include <string>
#include "net/instaweb/util/public/symbol_table.h"

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

  class Attribute {
   public:
    // TODO(sligocki): check sanity of values (ex: no stray quotes).
    //
    // TODO(jmarantz): arg 'quote' must be a static string, or NULL,
    // if quoting is not yet known (e.g. this is a synthesized attribute.
    // This is hard-to-describe and we should probably use an Atom for
    // the quote, and decide how to handle NULL.
    Attribute(Atom name, const char* value, const char* quote)
        : name_(name), quote_(quote) {
      set_value(value);
    }

    Atom name() const { return name_; }

    // Warning: the result of value() is still owned by
    // this, and will be invalidated by a subsequent call to set_value().
    // TODO(jmaessen): Fix ownership here, it's a mess.
    //   This creates knock-on problems with ImgFilter::ParseImgElement
    //   and with CssFilter::ParseCssElement.
    const char* value() const { return value_.get(); }
    const char* quote() const { return quote_; }

    // Modify value of attribute (eg to rewrite dest of src or href).
    // As with the constructor, copies the string in, so caller retains
    // ownership of value.
    void set_value(const char *value) {
      value_.reset((value == NULL) ? NULL : strdup(value));
    }

    // See comment about quote on constructor for Attribute.
    void set_quote(const char *quote) {
      quote_ = quote;
    }

   private:
    Atom name_;
    scoped_ptr_malloc<char> value_;
    const char* quote_;
  };

  ~HtmlElement();

  void AddAttribute(Atom name, const char* value, const char* quote) {
    attributes_.push_back(new Attribute(name, value, quote));
  }

  // Look for attribute by interned name.  NULL if no attribute exists.
  const Attribute* FirstAttributeWithName(Atom name) const;
  Attribute* FirstAttributeWithName(Atom name) {
    const HtmlElement* const_this = this;
    const Attribute* result = const_this->FirstAttributeWithName(name);
    return const_cast<Attribute*>(result);
  }

  // Replaces an existing attribute with a new value, returning false if
  // the attribute was not found.
  bool ReplaceAttribute(Atom name, const char* value);

  // Small integer uniquely identifying the HTML element, primarily
  // for debugging.
  void set_sequence(int sequence) { sequence_ = sequence; }

  Atom tag() const {return tag_;}
  int attribute_size() const {return attributes_.size(); }
  const Attribute& attribute(int i) const { return *attributes_[i]; }
  Attribute& attribute(int i) { return *attributes_[i]; }

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
  HtmlElement(Atom tag, const HtmlEventListIterator& begin,
      const HtmlEventListIterator& end);

  int sequence_;
  Atom tag_;
  std::vector<Attribute*> attributes_;
  HtmlEventListIterator begin_;
  HtmlEventListIterator end_;
  CloseStyle close_style_;
  int begin_line_number_;
  int end_line_number_;
};

}  // namespace net_instaweb

#endif  // NET_INSTAWEB_HTMLPARSE_PUBLIC_HTML_ELEMENT_H_
