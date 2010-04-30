// Copyright 2010 Google Inc. All Rights Reserved.
// Author: jmaessen@google.com (Jan Maessen)

#include "net/instaweb/rewriter/public/html_attribute_quote_removal.h"
#include "net/instaweb/htmlparse/public/html_element.h"
#include "net/instaweb/htmlparse/public/html_parse.h"
#include "net/instaweb/util/public/string_util.h"

namespace {

// Explicit about signedness because we are
// loading a 0-indexed lookup table.
const unsigned char kNoQuoteChars[] =
    "abcdefghijklmnopqrstuvwxyz"
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "0123456789-._:";

}  // namespace

namespace net_instaweb {
// Remove quotes; see description in .h file.

HtmlAttributeQuoteRemoval::HtmlAttributeQuoteRemoval(HtmlParse* html_parse)
    : total_quotes_removed_(0),
      html_parse_(html_parse) {
  // In pidgin Python:
  //    needs_no_quotes[:] = false
  //    needs_no_quotes[kNoQuoteChars] = true
  memset(&needs_no_quotes_, 0, sizeof(needs_no_quotes_));
  for (int i = 0; kNoQuoteChars[i] != '\0'; ++i) {
    needs_no_quotes_[kNoQuoteChars[i]] = true;
  }
}

bool HtmlAttributeQuoteRemoval::NeedsQuotes(const char *val) {
  bool needs_quotes = false;
  for (int i = 0; val[i] != '\0'; ++i) {
    // Explicit cast to unsigned char ensures that our offset
    // into needs_no_quotes_ is positive.
    needs_quotes = !needs_no_quotes_[static_cast<unsigned char>(val[i])];
    if (needs_quotes) {
      break;
    }
  }
  return needs_quotes;
}

void HtmlAttributeQuoteRemoval::StartElement(HtmlElement* element) {
  int rewritten = 0;
  for (int i = 0; i < element->attribute_size(); ++i) {
    HtmlElement::Attribute& attr = element->attribute(i);
    if (attr.quote() != NULL && attr.quote()[0] != '\0' &&
        !NeedsQuotes(attr.value())) {
      attr.set_quote("");
      rewritten++;
    }
  }
  if (rewritten > 0) {
    total_quotes_removed_ += rewritten;
    const char* plural = (rewritten == 1) ? "" : "s";
    html_parse_->InfoHere("Scrubbed quotes from %d attribute%s",
                          rewritten, plural);
  }
}

}  // namespace net_instaweb
