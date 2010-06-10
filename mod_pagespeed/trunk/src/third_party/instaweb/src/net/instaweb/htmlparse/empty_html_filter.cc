// Copyright 2010 and onwards Google Inc.
// Author: jmarantz@google.com (Joshua Marantz)

#include "public/empty_html_filter.h"

namespace net_instaweb {

EmptyHtmlFilter::EmptyHtmlFilter() {
}

EmptyHtmlFilter::~EmptyHtmlFilter() {
}

void EmptyHtmlFilter::StartDocument() {
}

void EmptyHtmlFilter::EndDocument() {
}

void EmptyHtmlFilter::StartElement(HtmlElement* element) {
}

void EmptyHtmlFilter::EndElement(HtmlElement* element) {
}

void EmptyHtmlFilter::Cdata(HtmlCdataNode* cdata) {
}

void EmptyHtmlFilter::Comment(HtmlCommentNode* comment) {
}

void EmptyHtmlFilter::IEDirective(const std::string& directive) {
}

void EmptyHtmlFilter::Characters(HtmlCharactersNode* characters) {
}

void EmptyHtmlFilter::Directive(HtmlDirectiveNode* directive) {
}

void EmptyHtmlFilter::Flush() {
}

}  // namespace net_instaweb
