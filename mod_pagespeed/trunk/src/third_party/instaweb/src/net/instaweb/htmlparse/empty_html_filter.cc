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

void EmptyHtmlFilter::Cdata(const std::string& cdata) {
}

void EmptyHtmlFilter::Comment(const std::string& comment) {
}

void EmptyHtmlFilter::IEDirective(const std::string& directive) {
}

void EmptyHtmlFilter::Characters(const std::string& characters) {
}

void EmptyHtmlFilter::IgnorableWhitespace(const std::string& whitespace) {
}

void EmptyHtmlFilter::DocType(
    const std::string& name, const std::string& ext_id,
    const std::string& sys_id) {
}

void EmptyHtmlFilter::Directive(const std::string& directive) {
}

void EmptyHtmlFilter::Flush() {
}
}
