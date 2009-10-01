// Copyright 2009 Google Inc. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "pagespeed/html/html_formatter.h"

#include "base/logging.h"
#include "base/string_util.h"

namespace pagespeed {

namespace html {

HtmlFormatter::HtmlFormatter(std::ostream* output) :
    output_(output), level_(0), has_children_(false) {
}

HtmlFormatter::HtmlFormatter(std::ostream* output, int level) :
    output_(output), level_(level), has_children_(false) {
}

void HtmlFormatter::Indent(int level) {
  for (int i = 0; i < level; ++i) {
    *output_ << " ";
  }
}

void HtmlFormatter::DoneAddingChildren() {
  if (has_children_ && level_ >= 2) {
    Indent(level_ - 1);
    *output_ << "</ul>" << std::endl;
  }
}

Formatter* HtmlFormatter::NewChild(
    const std::string& format_str,
    const std::vector<const Argument*>& arguments) {
  if (!has_children_) {
    has_children_ = true;
    if (level_ >= 2) {
      Indent(level_ - 1);
      *output_ << "<ul>" << std::endl;
    }
  }

  const std::string& str = Format(format_str, arguments);
  Indent(level_);
  switch (level_) {
    case 0:
      *output_ << "<h1>" << str << "</h1>" << std::endl;
      break;
    case 1:
      *output_ << "<h2>" << str << "</h2>" << std::endl;
      break;
    default:
      *output_ << "<li>" << str << "</li>" << std::endl;
      break;
  }

  return new HtmlFormatter(output_, level_ + 1);
}

std::string HtmlFormatter::Format(
    const std::string& format_str,
    const std::vector<const Argument*>& arguments) {
  std::vector<std::string> subst;

  for (std::vector<const Argument*>::const_iterator iter = arguments.begin(),
           end = arguments.end();
       iter != end;
       ++iter) {
    const Argument& arg = **iter;
    switch (arg.type()) {
      case Argument::URL:
        subst.push_back("<a href=\"" + arg.string_value() + "\">" +
                        arg.string_value() + "</a>");
        break;
      case Argument::STRING:
        subst.push_back(arg.string_value());
        break;
      case Argument::INTEGER:
        subst.push_back(IntToString(arg.int_value()));
        break;
      case Argument::BYTES:
        char buffer[100];
        snprintf(buffer, arraysize(buffer), "%.1fKiB",
                 arg.int_value() / 1024.0f);
        subst.push_back(buffer);
        break;
      default:
        CHECK(false);
        break;
    }
  }

  return ReplaceStringPlaceholders(format_str, subst, NULL);
}

}  // namespace html

}  // namespace pagespeed
