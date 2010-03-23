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

#include "html_rewriter/html_parser_message_handler.h"

#include "base/logging.h"
#include "base/string_util.h"
#include "mod_spdy/apache/log_message_handler.h"

namespace html_rewriter {
void HtmlParserMessageHandler::WarningV(
    const char* file, int line, const char *msg, va_list args) {
  LOG(WARNING) << file << ":" << line << ": " << Format(msg, args);
}

void HtmlParserMessageHandler::ErrorV(
    const char* file, int line, const char *msg, va_list args) {
  LOG(ERROR) << file << ":" << line << ": " << Format(msg, args);
}

void HtmlParserMessageHandler::FatalErrorV(
    const char* file, int line, const char* msg, va_list args) {
  LOG(FATAL) << file << ":" << line << ": " << Format(msg, args);
}

const std::string& HtmlParserMessageHandler::Format(const char* msg,
                                                    va_list args) {
  buffer_.clear();

  // Ignore the name of this routine: it formats with vsnprintf.
  // See base/string_util.cc.
  StringAppendV(&buffer_, msg, args);
  return buffer_;
}

}  // namespace html_rewriter
