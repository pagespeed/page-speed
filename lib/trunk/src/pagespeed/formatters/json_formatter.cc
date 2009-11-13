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

#include "pagespeed/formatters/json_formatter.h"

#include "base/logging.h"
#include "base/string_util.h"

namespace {

std::string QuotedJsonString(std::string str) {
  std::string quoted("\"");
  for (std::string::const_iterator i = str.begin(); i != str.end(); ++i) {
    switch (*i) {
      case '"':
        quoted.append("\\\"");
        break;
      case '\\':
        quoted.append("\\\\");
        break;
      case '\f':
        quoted.append("\\f");
        break;
      case '\n':
        quoted.append("\\n");
        break;
      case '\r':
        quoted.append("\\r");
        break;
      case '\t':
        quoted.append("\\t");
        break;
      default:
        // Unicode escape ASCII control and Extended ASCII characters.
        if (*i < 0x20 || *i >= 0x7F) {
          quoted.append(StringPrintf("\\u%04x", static_cast<int>(*i)));
        } else {
          quoted.push_back(*i);
        }
        break;
    }
  }
  quoted.push_back('"');
  return quoted;
}

std::string StringElement(bool &comma, std::string &str) {
  std::string result;
  if (!str.empty()) {
    if (comma) {
      result.push_back(',');
    }
    result.append("{\"type\":\"str\",\"value\":");
    result.append(QuotedJsonString(str));
    result.push_back('}');
    comma = true;
    str.clear();
  }
  return result;
}

std::string UrlElement(bool &comma, const std::string &url) {
  std::string result;
  if (comma) {
    result.push_back(',');
  }
  result.append("{\"type\":\"url\",\"value\":");
  result.append(QuotedJsonString(url));
  result.push_back('}');
  comma = true;
  return result;
}

}

namespace pagespeed {

namespace formatters {

JsonFormatter::JsonFormatter(std::ostream* output) :
    output_(output), level_(0), has_children_(false) {
}

JsonFormatter::JsonFormatter(std::ostream* output, int level) :
    output_(output), level_(level), has_children_(false) {
}

void JsonFormatter::DoneAddingChildren() {
  if (has_children_) {
    *output_ << "]";
  }
  if (level_ > 0) {
    *output_ << "}";
  } else {
    *output_ << "\n";
  }
}

Formatter* JsonFormatter::AddHeader(const std::string& header, int score) {
  Argument arg(Argument::STRING, header);
  Formatter* child_formatter = AddChild("$1", arg);
  *output_ << ",\"score\":" << score;
  return child_formatter;
}

Formatter* JsonFormatter::NewChild(
    const std::string& format_str,
    const std::vector<const Argument*>& arguments) {
  if (has_children_) {
    *output_ << ",";
  } else if (level_ > 0) {
    *output_ << ",\"children\":[";
  } else {
    *output_ << "[";
  }
  has_children_ = true;

  *output_ << "\n{\"format\":[";

  bool needs_comma = false;
  std::string str;
  for (std::string::const_iterator i = format_str.begin();
       i != format_str.end(); ++i) {
    if ('$' == *i) {
      if (i + 1 != format_str.end()) {
        ++i;
        DCHECK('$' == *i || '1' <= *i) << "Invalid placeholder: " << *i;
        if ('$' == *i) {
          str.push_back('$');
        } else {
          const int index = *i - '1';
          const Argument& arg = *arguments.at(index);
          switch (arg.type()) {
            case Argument::URL:
              *output_ << StringElement(needs_comma, str);
              *output_ << UrlElement(needs_comma, arg.string_value());
              break;
            case Argument::STRING:
              str.append(arg.string_value());
              break;
            case Argument::INTEGER:
              str.append(IntToString(arg.int_value()));
              break;
            case Argument::BYTES:
              str.append(StringPrintf("%.1fKiB", arg.int_value() / 1024.0f));
              break;
            default:
              CHECK(false);
              break;
          }
        }
      }
    } else {
      str.push_back(*i);
    }
  }
  *output_ << StringElement(needs_comma, str);

  *output_ << "]";

  return new JsonFormatter(output_, level_ + 1);
}

}  // namespace formatters

}  // namespace pagespeed
