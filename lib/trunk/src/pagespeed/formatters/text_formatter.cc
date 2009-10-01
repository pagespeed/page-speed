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

#include "pagespeed/formatters/text_formatter.h"

#include "base/logging.h"
#include "base/string_util.h"

namespace pagespeed {

namespace formatters {

TextFormatter::TextFormatter(std::ostream* output) :
    output_(output), level_(0) {
}

TextFormatter::TextFormatter(std::ostream* output, int level) :
    output_(output), level_(level) {
}

void TextFormatter::Indent(int level) {
  *output_ << std::string(2 * level, ' ');
}

void TextFormatter::DoneAddingChildren() {
}

Formatter* TextFormatter::NewChild(
    const std::string& format_str,
    const std::vector<const Argument*>& arguments) {

  const std::string str = Format(format_str, arguments);

  Indent(level_);
  switch (level_) {
    case 0:
      *output_ << "_" << str << "_" << std::endl;
      break;
    case 1:
      *output_ << str << std::endl;
      break;
    default:
      *output_ << "* " << str << std::endl;
      break;
  }

  return new TextFormatter(output_, level_ + 1);
}

std::string TextFormatter::Format(
    const std::string& format_str,
    const std::vector<const Argument*>& arguments) {
  std::vector<std::string> subst;

  for (std::vector<const Argument*>::const_iterator iter = arguments.begin(),
           end = arguments.end();
       iter != end;
       ++iter) {
    const Argument& arg = **iter;
    switch (arg.type()) {
      case Argument::STRING:
      case Argument::URL:
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

}  // namespace formatters

}  // namespace pagespeed
