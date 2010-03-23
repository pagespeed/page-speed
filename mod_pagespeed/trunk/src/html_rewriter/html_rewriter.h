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

#ifndef HTML_REWRITER_HTML_REWRITER_H_
#define HTML_REWRITER_HTML_REWRITER_H_

#include <string>

#include "html_rewriter/html_parser_message_handler.h"
#include "net/instaweb/htmlparse/public/html_parse.h"
#include "net/instaweb/htmlparse/public/html_writer_filter.h"
#include "net/instaweb/htmlparse/public/writer.h"

namespace html_rewriter {

// A string writer for HtmlWriterFilter to output the html to string.
class StringWriter : public net_instaweb::Writer {
 public:
  // Construct the writer with a pointer of string object as output.
  explicit StringWriter(std::string* str);
  // Append str to output.
  virtual bool Write(const char* str, int len,
                     MessageHandler* message_handler);
  // Flush does nothing in this StringWriter.
  virtual bool Flush(MessageHandler* message_handler);

 private:
  std::string* str_;
};

class HtmlRewriter {
 public:
  explicit HtmlRewriter(const std::string& url, std::string* output);
  // Rewrite input using internal StringWriter.
  void Rewrite(const char* input, int size);
  void Rewrite(const std::string& input) {
    Rewrite(input.data(), input.size());
  }
  // Flush the re-written content to output.
  void Flush();
  // Flush and finish the re-write.
  void Finish();

  const std::string& get_url() const {
    return url_;
  };
  const std::string& set_url(const std::string& url) {
    url_ = url;
  };

 private:
  std::string url_;
  HtmlParserMessageHandler message_handler_;
  net_instaweb::HtmlParse html_parse_;
  net_instaweb::HtmlWriterFilter html_write_filter_;
  StringWriter string_writer_;
};

}  // namespace html_rewriter

#endif  // HTML_REWRITER_HTML_REWRITER_H_
