// Copyright 2010 and onwards Google Inc.
// Author: jmarantz@google.com (Joshua Marantz)

#include "net/instaweb/htmlparse/public/html_writer_filter.h"

#include <assert.h>
#include "net/instaweb/htmlparse/public/html_parse.h"
#include "net/instaweb/htmlparse/public/html_element.h"
#include <string>
#include "net/instaweb/util/public/writer.h"

namespace net_instaweb {

static const int kDefaultMaxColumn = -1;

HtmlWriterFilter::HtmlWriterFilter(HtmlParse* html_parse)
    : writer_(NULL),
      write_errors_(0) {
  html_parse_ = html_parse;
  lazy_close_element_ = NULL;

  // Pre-intern a set of common symbols that can be used for
  // fast comparisons when matching tags, and for pointer-based
  // hash-tables.
  symbol_a_ = html_parse->Intern("a");
  symbol_link_ = html_parse->Intern("link");
  symbol_href_ = html_parse->Intern("href");
  symbol_img_ = html_parse->Intern("img");
  symbol_script_ = html_parse->Intern("script");
  symbol_src_ = html_parse->Intern("src");
  symbol_alt_ = html_parse->Intern("alt");
  max_column_ = kDefaultMaxColumn;
  column_ = 0;
}

HtmlWriterFilter::~HtmlWriterFilter() {
}

void HtmlWriterFilter::EmitBytes(const char* str, int size) {
  if (lazy_close_element_ != NULL) {
    lazy_close_element_ = NULL;
    if (!writer_->Write(">", 1, html_parse_->message_handler())) {
      ++write_errors_;
    }
    ++column_;
  }

  // Search backward from the end for the last occurrence of a newline.
  column_ += size;  // if there are no newlines, bump up column counter.
  for (int i = size - 1; i >= 0; --i) {
    if (str[i] == '\n') {
      column_ = size - i - 1;  // found a newline; so reset the column.
      break;
    }
  }
  if (!writer_->Write(str, size, html_parse_->message_handler())) {
    ++write_errors_;
  }
}

void HtmlWriterFilter::StartElement(HtmlElement* element) {
  EmitBytes("<");
  EmitBytes(element->tag().c_str());
  bool last_is_unquoted = false;
  for (int i = 0; i < element->attribute_size(); ++i) {
    const HtmlElement::Attribute& attribute = element->attribute(i);
    // If the column has grown too large, insert a newline.  It's always safe
    // to insert whitespace in the middle of tag parameters.
    int attr_length = 1 + attribute.name().size();
    if (max_column_ > 0) {
      if (attribute.value() != NULL) {
        attr_length += 1 + strlen(attribute.value());
      }
      if ((column_ + attr_length) > max_column_) {
        EmitBytes("\n", 1);
      }
    }
    EmitBytes(" ");
    EmitBytes(attribute.name().c_str());
    last_is_unquoted = false;
    if (attribute.value() != NULL) {
      EmitBytes("=", 1);
      EmitBytes(attribute.quote());
      EmitBytes(attribute.value());
      EmitBytes(attribute.quote());
      last_is_unquoted = (attribute.quote()[0] == '\0');
    }
  }

  // If the last element was not quoted, then delimit with a space.
  if (last_is_unquoted) {
    EmitBytes(" ");
  }

  // Attempt to briefly terminate any legal tag that was explicitly terminated
  // in the input.  Note that a rewrite pass might have injected events
  // between the begin/end of an element that was closed briefly in the input
  // html.  In that case it cannot be closed briefly.  It is up to this
  // code to validate BRIEF_CLOSE on each element.
  //
  // TODO(jmarantz): Add a rewrite pass that morphs EXPLICIT_CLOSE into 'brief'
  // when legal.  Such a change will introduce textual diffs between
  // input and output html that would cause htmlparse unit tests to require
  // a regold.  But the changes could be validated with the normalizer.
  if (GetCloseStyle(element) == HtmlElement::BRIEF_CLOSE) {
    lazy_close_element_ = element;
  } else {
    EmitBytes(">", 1);
  }
}

// Compute the tag-closing style for an element. If the style was specified
// on construction, then we use that.  If the element was synthesized by
// a rewrite pass, then it's stored as AUTO_CLOSE, and we can determine
// whether the element is briefly closable or implicitly closed.
HtmlElement::CloseStyle HtmlWriterFilter::GetCloseStyle(HtmlElement* element) {
  HtmlElement::CloseStyle style = element->close_style();
  if (style == HtmlElement::AUTO_CLOSE) {
    Atom tag = element->tag();
    if (html_parse_->IsImplicitlyClosedTag(tag)) {
      style = HtmlElement::IMPLICIT_CLOSE;
    } else if (html_parse_->TagAllowsBriefTermination(tag)) {
      style = HtmlElement::BRIEF_CLOSE;
    } else {
      style = HtmlElement::EXPLICIT_CLOSE;
    }
  }
  return style;
}

void HtmlWriterFilter::EndElement(HtmlElement* element) {
  HtmlElement::CloseStyle style = GetCloseStyle(element);
  switch (style) {
    case HtmlElement::AUTO_CLOSE:
      // This cannot happen because GetCloseStyle prevents won't
      // return AUTO_CLOSE.
      assert(0);
      break;
    case HtmlElement::IMPLICIT_CLOSE:
      // Nothing new to write; the ">" was written in StartElement
      break;
    case HtmlElement::BRIEF_CLOSE:
      // even if the element is briefly closeable, if more text
      // got written after the element open, then we must
      // explicitly close it, so we fall through.
      if (lazy_close_element_ == element) {
        lazy_close_element_ = NULL;
        EmitBytes("/>", 2);
        break;
      }
      // fall through
    case HtmlElement::EXPLICIT_CLOSE:
      EmitBytes("</", 2);
      EmitBytes(element->tag().c_str());
      EmitBytes(">", 1);
      break;
    case HtmlElement::UNCLOSED:
      // Nothing new to write; the ">" was written in StartElement
      break;
  }
}

void HtmlWriterFilter::Characters(const std::string& chars) {
  EmitBytes(chars);
}

void HtmlWriterFilter::Cdata(const std::string& cdata) {
  EmitBytes(cdata);
}

void HtmlWriterFilter::IgnorableWhitespace(const std::string& whitespace) {
  EmitBytes(whitespace);
}

void HtmlWriterFilter::Comment(const std::string& value) {
  EmitBytes("<!--");
  EmitBytes(value);
  EmitBytes("-->");
}

void HtmlWriterFilter::IEDirective(const std::string& value) {
  EmitBytes("<!--");
  EmitBytes(value);
  EmitBytes("-->");
}

void HtmlWriterFilter::DocType(
    const std::string& name, const std::string& ext_id,
    const std::string& sys_id) {
  EmitBytes("<!doctype ", 10);
  EmitBytes(name);
  if (!ext_id.empty()) {
    EmitBytes(" PUBLIC \"", 9);
    EmitBytes(ext_id);
    EmitBytes("\"", 1);
  }
  if (!sys_id.empty()) {
    EmitBytes(" \"", 2);
    EmitBytes(sys_id);
    EmitBytes("\"", 1);
  }
  EmitBytes(">");
}

void HtmlWriterFilter::Directive(const std::string& directive) {
  EmitBytes("<!", 2);
  EmitBytes(directive);
  EmitBytes(">", 1);
}

void HtmlWriterFilter::StartDocument() {
  column_ = 0;
  lazy_close_element_ = NULL;
}

void HtmlWriterFilter::EndDocument() {
}

void HtmlWriterFilter::Flush() {
  if (!writer_->Flush(html_parse_->message_handler())) {
    ++write_errors_;
  }
}

}  // namespace net_instaweb
