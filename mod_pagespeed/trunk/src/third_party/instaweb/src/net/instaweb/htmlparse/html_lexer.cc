// Copyright 2010 and onwards Google Inc.
// Author: jmarantz@google.com (Joshua Marantz)

#include "net/instaweb/htmlparse/html_lexer.h"
#include <assert.h>
#include <ctype.h>
#include <string.h>
#include "net/instaweb/htmlparse/html_event.h"
#include "net/instaweb/htmlparse/public/html_element.h"
#include "net/instaweb/htmlparse/public/html_parse.h"

namespace {
// These tags can be specified in documents without a brief "/>",
// or an explicit </tag>, according to the Chrome Developer Tools console.
//
// TODO(jmarantz): Check out
// http://www.whatwg.org/specs/web-apps/current-work/multipage/
// syntax.html#optional-tags
const char* kImplicitlyClosedHtmlTags[] = {
  "meta", "input", "link", "br", "img",
  NULL
};

// These tags cannot be closed using the brief syntax; they must
// be closed by using an explicit </TAG>.
static const char* kNonBriefTerminatedTags[] = {
  "script", "a", "div", "span", "iframe", "style", "textarea",
  NULL
};
}

// TODO(jmarantz): support multi-byte encodings
// TODO(jmarantz): emit close-tags immediately for selected html tags,
//   rather than waiting for the next explicit close-tag to force a rebalance.
//   See http://www.whatwg.org/specs/web-apps/current-work/multipage/
//   syntax.html#optional-tags

namespace net_instaweb {

HtmlLexer::HtmlLexer(HtmlParse* html_parse)
    : html_parse_(html_parse),
      state_(START),
      attr_quote_(""),
      has_attr_value_(false),
      element_(NULL),
      line_(1) {
  for (const char** p = kImplicitlyClosedHtmlTags; *p != NULL; ++p) {
    implicitly_closed_.insert(html_parse->Intern(*p));
  }
  for (const char** p = kNonBriefTerminatedTags; *p != NULL; ++p) {
    non_brief_terminated_tags_.insert(html_parse->Intern(*p));
  }
}

HtmlLexer::~HtmlLexer() {
}

void HtmlLexer::EvalStart(char c) {
  if (c == '<') {
    literal_.resize(literal_.size() - 1);
    EmitLiteral();
    literal_ += c;
    state_ = TAG;
  } else {
    state_ = START;
  }
}

#define LEGAL_TAG_CHAR(c) (isalnum(c) || ((c) == '-') || ((c) == '#'))
#define LEGAL_ATTR_NAME_CHAR(c) (((c) != '=') && ((c) != '>') && ((c) != '/') \
                                 && !isspace(c))

// Handle the case where "<" was recently parsed.
void HtmlLexer::EvalTag(char c) {
  if (c == '/') {
    state_ = TAG_CLOSE;
  } else if (LEGAL_TAG_CHAR(c)) {   // "<x"
    state_ = TAG_OPEN;
    token_ += c;
  } else if (c == '!') {
    state_ = COMMENT_START1;
  } else {
    //  Illegal tag syntax; just pass it through as raw characters
    html_parse_->Error(filename_.c_str(), line_,
        "Invalid tag syntax: unexpected sequence `<%c'", c);
    EvalStart(c);
  }
}

// Handle the case where "<x" was recently parsed.  We will stay in this
// state as long as we keep seeing legal tag characters, appending to
// token_ for each character.
void HtmlLexer::EvalTagOpen(char c) {
  if (LEGAL_TAG_CHAR(c)) {
    token_ += c;
  } else if (c == '>') {
    EmitTagOpen(true);
  } else if (c == '<') {
    // Chrome transforms "<tag<tag>" into "<tag><tag>";
    html_parse_->Error(filename_.c_str(), line_,
        "Invalid tag syntax: expected close tag before opener");
    EmitTagOpen(true);
    EvalStart(c);
  } else if (c == '/') {
    state_ = TAG_BRIEF_CLOSE;
  } else if (isspace(c)) {
    state_ = TAG_ATTRIBUTE;
  } else {
    // Some other punctuation.  Not sure what to do.  Let's run this
    // on the web and see what breaks & decide what to do.  E.g. "<x&"
    html_parse_->Error(filename_.c_str(), line_,
        "Invalid tag syntax: expected close tag before opener");
  }
}

// Handle several cases of seeing "/" in the middle of a tag, but after
// the identifier has been completed.  Examples: "<x /" or "<x y/" or "x y=/z".
void HtmlLexer::EvalTagBriefCloseAttr(char c) {
  if (c == '>') {
    FinishAttribute(c, has_attr_value_, true);
  } else if (isspace(c)) {
    // "<x y/ ".  This can lead to "<x y/ z" where z would be
    // a new attribute, or "<x y/ >" where the tag would be
    // closed without adding a new attribute.  In either case,
    // we will be completing this attribute.
    //
    // TODO(jmarantz): what about "<x y/ =z>"?  I am not sure
    // sure if this matters, because testing that would require
    // a browser that could react to a named attribute with a
    // slash in the name (not the value).  But should we wind
    // up with 1 attributes or 2 for this case?  There are probably
    // more important questions, but if we ever need to answer that
    // one, this is the place.
    if (!attr_name_.empty()) {
      MakeAttribute(has_attr_value_);
    }
  } else {
    // Slurped www.google.com has
    //   <a href=/advanced_search?hl=en>Advanced Search</a>
    // So when we first see the "/" it looks like it might
    // be a brief-close, .e.g. <a href=/>.  But when we see
    // that what follows the '/' is not '>' then we know it's
    // just part off the attribute name or value.  So there's
    // no need to even warn.
    if (has_attr_value_) {
      attr_value_ += '/';
      state_ = TAG_ATTR_VAL;
      EvalAttrVal(c);
      // we know it's not the double-quoted or single-quoted versions
      // because then we wouldn't have let the '/' get us into the
      // brief-close state.
    } else {
      attr_name_ += '/';
      state_ = TAG_ATTR_NAME;
      EvalAttrName(c);
    }
  }
}

// Handle the case where "<x/" was recently parsed, where "x" can
// be any length tag identifier.  Note that we if we anything other
// than a ">" after this, we will just consider the "/" to be part
// of the tag identifier, and go back to the TAG_OPEN state.
void HtmlLexer::EvalTagBriefClose(char c) {
  if (c == '>') {
    EmitTagOpen(false);
    EmitTagBriefClose();
  } else {
    html_parse_->Error(filename_.c_str(), line_,
        "Invalid tag syntax: expected > after <xxx/ got `%c'", c);
    // Recover by returning to the mode from whence we came.
    assert(element_ != NULL);
    token_ += '/';
    state_ = TAG_OPEN;
    EvalTagOpen(c);
  }
}

// Handle the case where "</" was recently parsed.
void HtmlLexer::EvalTagClose(char c) {
  if (LEGAL_TAG_CHAR(c)) {  // "</x"
    token_ += c;
  } else if (c == '>') {
    EmitTagClose(HtmlElement::EXPLICIT_CLOSE);
  } else {
    html_parse_->Error(
        filename_.c_str(), line_,
        "Invalid tag syntax: expected `>' after `</%s' got `%c'",
        token_.c_str(), c);
    EmitTagClose(HtmlElement::EXPLICIT_CLOSE);
    EvalStart(c);
  }
}

// Handle the case where "<!x" was recently parsed, where x
// is any illegal tag identifier.  We stay in this state until
// we see the ">", accumulating the directive in token_.
void HtmlLexer::EvalDirective(char c) {
  if (c == '>') {
    EmitDirective();
  } else {
    token_ += c;
  }
}

// Handle the case where "<!" was recently parsed.
void HtmlLexer::EvalCommentStart1(char c) {
  if (c == '-') {
    state_ = COMMENT_START2;
  } else if (LEGAL_TAG_CHAR(c)) {  // "<!DOCTYPE ... >"
    state_ = DIRECTIVE;
    EvalDirective(c);
  } else {
    html_parse_->Error(filename_.c_str(), line_, "Invalid comment syntax");
    EmitLiteral();
    EvalStart(c);
  }
}

// Handle the case where "<!-" was recently parsed.
void HtmlLexer::EvalCommentStart2(char c) {
  if (c == '-') {
    state_ = COMMENT_BODY;
  } else {
    html_parse_->Error(filename_.c_str(), line_, "Invalid comment syntax");
    EmitLiteral();
    EvalStart(c);
  }
}

// Handle the case where "<!--" was recently parsed.  We will stay in
// this state until we see "-".  And even after that we may go back to
// this state if the "-" is not followed by "->".
void HtmlLexer::EvalCommentBody(char c) {
  if (c == '-') {
    state_ = COMMENT_END1;
  } else {
    token_ += c;
  }
}

// Handle the case where "-" has been parsed from a comment.  If we
// see another "-" then we go to CommentEnd2, otherwise we go back
// to the comment state.
void HtmlLexer::EvalCommentEnd1(char c) {
  if (c == '-') {
    state_ = COMMENT_END2;
  } else {
    // thought we were ending a comment cause we saw '-', but
    // now we changed our minds.   No worries mate.  That
    // fake-out dash was just part of the comment.
    token_ += '-';
    token_ += c;
    state_ = COMMENT_BODY;
  }
}

// Handle the case where "--" has been parsed from a comment.
void HtmlLexer::EvalCommentEnd2(char c) {
  if (c == '>') {
    EmitComment();
    state_ = START;
  } else {
    // thought we were ending a comment cause we saw '--', but
    // now we changed our minds.   No worries mate.  Those
    // fake-out dashes were just part of the comment.
    token_ += "--";
    token_ += c;
    state_ = COMMENT_BODY;
  }
}

// Handle the case where a script tag was started.  This is of lexical
// significance because we ignore all the special characters until we
// see "</script>".
void HtmlLexer::EvalScript(char c) {
  // Look explicitly for </script> in the literal buffer.
  // TODO(jmarantz): check for whitespace in unexpected places.
  if (c == '>') {
    int size_minus_9 = literal_.size() - 9;
    if ((size_minus_9 >= 0) &&
        (strcasecmp(literal_.c_str() + size_minus_9, "</script>") == 0)) {
      // The literal actually starts after the "<script>", and we will
      // also let it finish before, so chop it off.
      literal_.resize(size_minus_9);
      EmitLiteral();
      token_ = "script";
      EmitTagClose(HtmlElement::EXPLICIT_CLOSE);
    }
  }
}

// Emits raw uninterpreted characters.
void HtmlLexer::EmitLiteral() {
  if (!literal_.empty()) {
    html_parse_->AddEvent(new HtmlCharactersEvent(literal_));
    literal_.clear();
  }
  state_ = START;
}

void HtmlLexer::EmitComment() {
  literal_.clear();
  if ((token_.find("[if IE") != std::string::npos) &&
      (token_.find("<![endif]") != std::string::npos)) {
    html_parse_->AddEvent(new HtmlIEDirectiveEvent(token_));
  } else {
    html_parse_->AddEvent(new HtmlCommentEvent(token_));
  }
  token_.clear();
  state_ = START;
}

// If allow_implicit_close is true, and the element type is one which
// does not require an explicit termination in HTML, then we will
// automatically emit a matching 'element close' event.
void HtmlLexer::EmitTagOpen(bool allow_implicit_close) {
  literal_.clear();
  MakeElement();
  // ...
  html_parse_->AddElement(element_);
  if (strcmp(element_->tag(), "script") == 0) {
    state_ = SCRIPT;
  } else {
    state_ = START;
  }

  const char* tag = element_->tag();
  if (allow_implicit_close && IsImplicitlyClosedTag(tag)) {
    token_ = tag;
    EmitTagClose(HtmlElement::IMPLICIT_CLOSE);
  }

  element_ = NULL;
}

void HtmlLexer::EmitTagBriefClose() {
  HtmlElement* element = html_parse_->PopElement();
  html_parse_->CloseElement(element, HtmlElement::BRIEF_CLOSE);
  state_ = START;
}

static void toLower(std::string* str) {
  for (int i = 0, n = str->size(); i < n; ++i) {
    char& c = (*str)[i];
    c = tolower(c);
  }
}

void HtmlLexer::MakeElement() {
  if (element_ == NULL) {
    if (token_.empty()) {
      html_parse_->Error(
          filename_.c_str(), line_, "Making element with empty tag name");
    }
    toLower(&token_);
    element_ = html_parse_->NewElement(html_parse_->Intern(token_));
    token_.clear();
  }
}

void HtmlLexer::StartParse(const char* url) {
  line_ = 1;
  filename_ = url;
  has_attr_value_ = false;
  attr_quote_ = "";
  // clear buffers
}

void HtmlLexer::FinishParse() {
  if (!token_.empty()) {
    html_parse_->Error(filename_.c_str(), line_,
                       "End-of-file in mid-token: %s", token_.c_str());
    token_.clear();
  }
  if (!attr_name_.empty()) {
    html_parse_->Error(
        filename_.c_str(), line_, "End-of-file in mid-attribute-name: ",
        attr_name_.c_str());
    attr_name_.clear();
  }
  if (!attr_value_.empty()) {
    html_parse_->Error(
        filename_.c_str(), line_,
        "End-of-file in mid-attribute-value: ", attr_value_.c_str());
    attr_value_.clear();
  }

  if (!literal_.empty()) {
    EmitLiteral();
  }
}

void HtmlLexer::MakeAttribute(bool has_value) {
  assert(element_ != NULL);
  toLower(&attr_name_);
  const char* name = html_parse_->Intern(attr_name_);
  attr_name_.clear();
  const char* value = NULL;
  assert(has_value == has_attr_value_);
  if (has_value) {
    value = html_parse_->Intern(attr_value_);
    attr_value_.clear();
    has_attr_value_ = false;
  } else {
    assert(attr_value_.empty());
  }
  element_->AddAttribute(name, value, attr_quote_);
  attr_quote_ = "";
  state_ = TAG_ATTRIBUTE;
}

void HtmlLexer::EvalAttribute(char c) {
  MakeElement();
  attr_name_.clear();
  attr_value_.clear();
  if (c == '>') {
    EmitTagOpen(true);
  } else if (c == '/') {
    state_ = TAG_BRIEF_CLOSE_ATTR;
  } else if (LEGAL_ATTR_NAME_CHAR(c)) {
    attr_name_ += c;
    state_ = TAG_ATTR_NAME;
  } else if (!isspace(c)) {
    html_parse_->Error(
        filename_.c_str(), line_, "Unexpected char `%c' in attribute list", c);
  }
}

void HtmlLexer::EvalAttrName(char c) {
  if (c == '=') {
    state_ = TAG_ATTR_EQ;
    has_attr_value_ = true;
  } else if (isspace(c)) {
    FinishAttribute(c, false, false);
  } else if (LEGAL_ATTR_NAME_CHAR(c)) {
    attr_name_ += c;
  } else {
    FinishAttribute(c, false, false);
  }
}

void HtmlLexer::FinishAttribute(char c, bool has_value, bool brief_close) {
  if (isspace(c)) {
    MakeAttribute(has_value);
    state_ = TAG_ATTRIBUTE;
  } else if (c == '/') {
    // If / was seen terminating an attribute, without
    // the closing quote or whitespace, it might just be
    // part of a syntactically dubious attribute.  We'll
    // hold off completing the attribute till we see the
    // next character.
    state_ = TAG_BRIEF_CLOSE_ATTR;
  } else if (c == '>') {
    if (!attr_name_.empty()) {
      if (!brief_close &&
          (strcmp(attr_name_.c_str(), "/") == 0) && !has_value) {
        brief_close = true;
        attr_name_.clear();
        attr_value_.clear();
        has_attr_value_ = false;
      } else {
        MakeAttribute(has_value);
      }
    }
    EmitTagOpen(!brief_close);
    if (brief_close) {
      EmitTagBriefClose();
    }
  } else {
    // Some other funny character within a tag.  Probably can't
    // trust the tag at all.  Check the web and see when this
    // happens.
    html_parse_->Error(
        filename_.c_str(), line_, "Unexpected character in attribute: %c", c);
    EmitLiteral();
    state_ = START;
  }
}

void HtmlLexer::EvalAttrEq(char c) {
  if (LEGAL_TAG_CHAR(c)) {
    state_ = TAG_ATTR_VAL;
    attr_quote_ = "";
    EvalAttrVal(c);
  } else if (c == '"') {
    attr_quote_ = "\"";
    state_ = TAG_ATTR_VALDQ;
  } else if (c == '\'') {
    attr_quote_ = "'";
    state_ = TAG_ATTR_VALSQ;
  } else if (isspace(c)) {
    // ignore -- spaces are allowed between "=" and the value
  } else {
    FinishAttribute(c, true, false);
  }
}

void HtmlLexer::EvalAttrVal(char c) {
  if (isspace(c) || (c == '>') || (c == '/')) {
    FinishAttribute(c, true, false);
  } else {
    attr_value_ += c;
  }
}

void HtmlLexer::EvalAttrValDq(char c) {
  if (c == '"') {
    MakeAttribute(true);
  } else {
    attr_value_ += c;
  }
}

void HtmlLexer::EvalAttrValSq(char c) {
  if (c == '\'') {
    MakeAttribute(true);
  } else {
    attr_value_ += c;
  }
}

void HtmlLexer::EmitTagClose(HtmlElement::CloseStyle close_style) {
  HtmlElement* element = html_parse_->PopElement();

  // If there were unclosed tags on the stack, implicitly
  // close them until we get a match.
  toLower(&token_);

  if (element != NULL) {
    if (token_ != element->tag()) {
      html_parse_->Error(filename_.c_str(), line_,
          "Mismatching close-tag `%s', expecting `%s'",
          token_.c_str(), element->tag());
      EmitLiteral();
    } else {
      html_parse_->CloseElement(element, close_style);
    }
  } else {
    html_parse_->Error(filename_.c_str(), line_,
        "Unexpected close-tag `%s', no tags are open",
        token_.c_str());
    EmitLiteral();
  }

  literal_.clear();
  token_.clear();
  state_ = START;
}

void HtmlLexer::EmitDirective() {
  literal_.clear();
  html_parse_->AddEvent(new HtmlDirectiveEvent(token_));
  token_.clear();
  state_ = START;
}

void HtmlLexer::Parse(const char* text, int size) {
  for (int i = 0; i < size; ++i) {
    char c = text[i];
    if (c == '\n') {
      ++line_;
    }

    // By default we keep track of every byte as it comes in.
    // If we can't accurately parse it, we transmit it as
    // raw characters to be re-serialized without interpretation,
    // and good luck to the browser.  When we do successfully
    // parse something, we remove it from the literal.
    literal_ += c;

    switch (state_) {
      case START:                 EvalStart(c);               break;
      case TAG:                   EvalTag(c);                 break;
      case TAG_OPEN:              EvalTagOpen(c);             break;
      case TAG_CLOSE:             EvalTagClose(c);            break;
      case TAG_BRIEF_CLOSE:       EvalTagBriefClose(c);       break;
      case TAG_BRIEF_CLOSE_ATTR:  EvalTagBriefCloseAttr(c);   break;
      case COMMENT_START1:        EvalCommentStart1(c);       break;
      case COMMENT_START2:        EvalCommentStart2(c);       break;
      case COMMENT_BODY:          EvalCommentBody(c);         break;
      case COMMENT_END1:          EvalCommentEnd1(c);         break;
      case COMMENT_END2:          EvalCommentEnd2(c);         break;
      case TAG_ATTRIBUTE:         EvalAttribute(c);           break;
      case TAG_ATTR_NAME:         EvalAttrName(c);            break;
      case TAG_ATTR_EQ:           EvalAttrEq(c);              break;
      case TAG_ATTR_VAL:          EvalAttrVal(c);             break;
      case TAG_ATTR_VALDQ:        EvalAttrValDq(c);           break;
      case TAG_ATTR_VALSQ:        EvalAttrValSq(c);           break;
      case SCRIPT:                EvalScript(c);              break;
      case DIRECTIVE:             EvalDirective(c);           break;
    }
  }
}

bool HtmlLexer::IsImplicitlyClosedTag(const char* tag) const {
  return (implicitly_closed_.find(tag) != implicitly_closed_.end());
}

bool HtmlLexer::TagAllowsBriefTermination(const char* tag) const {
  return (non_brief_terminated_tags_.find(tag) ==
          non_brief_terminated_tags_.end());
}
}
