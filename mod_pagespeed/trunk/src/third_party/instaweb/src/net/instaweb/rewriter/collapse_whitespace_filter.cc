// Copyright 2010 and onwards Google Inc.
// Author: mdsteele@google.com (Matthew D. Steele)

#include "net/instaweb/rewriter/public/collapse_whitespace_filter.h"

#include "base/logging.h"
#include "net/instaweb/htmlparse/public/html_parse.h"
#include "net/instaweb/htmlparse/public/html_element.h"
#include "net/instaweb/htmlparse/public/html_node.h"
#include <string>

namespace {

// Tags within which we should never try to collapse whitespace (note that this
// is not _quite_ the same thing as kLiteralTags in html_lexer.cc):
const char* const kSensitiveTags[] = {"pre", "script", "style", "textarea"};

bool IsHtmlWhiteSpace(char ch) {
  // See http://www.w3.org/TR/html401/struct/text.html#h-9.1
  return ch == ' ' || ch == '\n' || ch == '\t' || ch == '\f';
}

// Append the input to the output with whitespace collapsed.
void CollapseWhitespace(const std::string& input, std::string* output) {
  char whitespace = '\0';
  for (std::string::const_iterator iter = input.begin(), end = input.end();
       iter != end; ++iter) {
    const char ch = *iter;
    if (IsHtmlWhiteSpace(ch)) {
      if (whitespace == '\0' || ch == '\n') {
        whitespace = ch;
      }
    } else {
      if (whitespace != '\0') {
        *output += whitespace;
        whitespace = '\0';
      }
      *output += ch;
    }
  }
  if (whitespace != '\0') {
    *output += whitespace;
  }
}

}  // namespace

namespace net_instaweb {

CollapseWhitespaceFilter::CollapseWhitespaceFilter(HtmlParse* html_parse)
    : html_parse_(html_parse) {
  for (size_t i = 0; i < arraysize(kSensitiveTags); ++i) {
    sensitive_tags_.insert(html_parse->Intern(kSensitiveTags[i]));
  }
}

void CollapseWhitespaceFilter::StartDocument() {
  atom_stack_.clear();
}

void CollapseWhitespaceFilter::StartElement(HtmlElement* element) {
  const Atom tag = element->tag();
  if (sensitive_tags_.count(tag) > 0) {
    atom_stack_.push_back(tag);
  }
}

void CollapseWhitespaceFilter::EndElement(HtmlElement* element) {
  const Atom tag = element->tag();
  if (!atom_stack_.empty() && tag == atom_stack_.back()) {
    atom_stack_.pop_back();
  } else {
    DCHECK(sensitive_tags_.count(tag) == 0);
  }
}

void CollapseWhitespaceFilter::Characters(HtmlCharactersNode* characters) {
  if (atom_stack_.empty()) {
    std::string minified;
    CollapseWhitespace(characters->contents(), &minified);
    html_parse_->ReplaceNode(characters,
                             html_parse_->NewCharactersNode(minified));
  }
}

}  // namespace net_instaweb
