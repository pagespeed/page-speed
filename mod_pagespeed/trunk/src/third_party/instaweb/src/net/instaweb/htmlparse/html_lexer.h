// Copyright 2010 and onwards Google Inc.
// Author: jmarantz@google.com (Joshua Marantz)

#ifndef NET_INSTAWEB_HTMLPARSE_HTML_LEXER_H_
#define NET_INSTAWEB_HTMLPARSE_HTML_LEXER_H_

#include <stdarg.h>
#include <set>
#include <string>
#include "net/instaweb/htmlparse/public/html_element.h"

namespace net_instaweb {

// Constructs a re-entrant HTML lexer.  This lexer minimally parses tags,
// attributes, and comments.  It is intended to parse the Wild West of the
// Web.  It's designed to be tolerant of syntactic transgressions, merely
// passing through unparseable chunks as Characters.
//
// TODO(jmarantz): refactor this with html_parse, so that this class owns
// the symbol table and the event queue, and no longer needs to mutually
// depend on HtmlParse.  That will make it easier to unit-test.
class HtmlLexer {
 public:
  explicit HtmlLexer(HtmlParse* html_parse);
  ~HtmlLexer();

  // Initialize a new parse session, establishing name used for error messages.
  void StartParse(const char* url_or_filename);

  // Parse a chunk of text, adding events to the parser by calling
  // html_parse_->AddEvent(...).
  void Parse(const char* text, int size);

  // Completes parsing a file or url, reporting any leftover text as a final
  // HtmlCharacterEvent.
  void FinishParse();

  // Determines whether a tag should be terminated in HTML.
  bool IsImplicitlyClosedTag(const char* tag) const;

  // Determines whether a tag can be terminated briefly (e.g. <tag/>)
  bool TagAllowsBriefTermination(const char* tag) const;

 private:
  void EvalStart(char c);
  void EvalTag(char c);
  void EvalTagOpen(char c);
  void EvalTagClose(char c);
  void EvalTagBriefClose(char c);
  void EvalTagBriefCloseAttr(char c);
  void EvalCommentStart1(char c);
  void EvalCommentStart2(char c);
  void EvalCommentBody(char c);
  void EvalCommentEnd1(char c);
  void EvalCommentEnd2(char c);
  void EvalAttribute(char c);
  void EvalAttrName(char c);
  void EvalAttrEq(char c);
  void EvalAttrVal(char c);
  void EvalAttrValSq(char c);
  void EvalAttrValDq(char c);
  void EvalLiteralTag(char c);
  void EvalDirective(char c);

  void MakeElement();
  void MakeAttribute(bool has_value);
  void FinishAttribute(char c, bool has_value, bool brief_close);

  void EmitComment();
  void EmitLiteral();
  void EmitTagOpen(bool allow_implicit_close);
  void EmitTagClose(HtmlElement::CloseStyle close_style);
  void EmitTagBriefClose();
  void EmitDirective();

  // The lexer is implemented as a pure state machine.  There is
  // no lookahead.  The state is understood primarily in this
  // enum, although there are a few state flavors that are managed
  // by the other member variables, notably: has_attr_value_ and
  // attr_name_.empty().  Those could be eliminated by adding
  // a few more explicit states.
  enum State {
    START,
    TAG,                   // "<"
    TAG_CLOSE,             // "</"
    TAG_OPEN,              // "<x"
    TAG_BRIEF_CLOSE,       // "<x/"
    TAG_BRIEF_CLOSE_ATTR,  // "<x /" or "<x y/" or "x y=/z" etc
    COMMENT_START1,        // "<!"
    COMMENT_START2,        // "<!-"
    COMMENT_BODY,          // "<!--"
    COMMENT_END1,          // "-"
    COMMENT_END2,          // "--"
    TAG_ATTRIBUTE,         // "<x "
    TAG_ATTR_NAME,         // "<x y"
    TAG_ATTR_EQ,           // "<x y="
    TAG_ATTR_VAL,          // "<x y=x" value terminated by whitespace or >
    TAG_ATTR_VALDQ,        // '<x y="' value terminated by double-quote
    TAG_ATTR_VALSQ,        // "<x y='" value terminated by single-quote
    LITERAL_TAG,           // "<script " or "<iframe "
    DIRECTIVE              // "<!x"
  };

  HtmlParse* html_parse_;
  State state_;
  std::string token_;       // accmulates tag names and comments
  std::string literal_;     // accumulates raw text to pass through
  std::string attr_name_;   // accumulates attribute name
  std::string attr_value_;  // accumulates attribute value
  const char* attr_quote_;  // accumulates quote used to delimit attribute
  bool has_attr_value_;     // distinguishes <a n=> from <a n>
  HtmlElement* element_;    // current element; used to collect attributes
  int line_;
  std::string filename_;
  std::string literal_close_;  // specific tag go close, e.g </script>

  std::set<const char*> implicitly_closed_;
  std::set<const char*> non_brief_terminated_tags_;
};
}

#endif  // NET_INSTAWEB_HTMLPARSE_HTML_LEXER_H_
