// Copyright 2010 Google Inc.
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

#include <string>

#include "html_rewriter/html_rewriter.h"

namespace html_rewriter {

StringWriter::StringWriter(std::string* str) : str_(str) {
}

bool StringWriter::Write(const char* str, int len,
                   MessageHandler* /*message_handler*/) {
  str_->append(str, len);
  return true;  // No error will happen here.
}

bool StringWriter::Flush(MessageHandler* /*message_handler*/) {
  // No errors can happen
  return true;
}

HtmlRewriter::HtmlRewriter(const std::string& url, std::string* output)
    : url_(url),
      message_handler_(),
      html_parse_(&message_handler_),
      html_write_filter_(&html_parse_),
      string_writer_(output) {
  html_parse_.AddFilter(&html_write_filter_);
  html_parse_.StartParse(url_.c_str());
  html_write_filter_.set_writer(&string_writer_);
}

void HtmlRewriter::Finish() {
  html_parse_.FinishParse();
}

void HtmlRewriter::Flush() {
  html_parse_.Flush();
}

void HtmlRewriter::Rewrite(const char* input, int size) {
  html_parse_.ParseText(input, size);
}

}  // namespace html_rewriter
