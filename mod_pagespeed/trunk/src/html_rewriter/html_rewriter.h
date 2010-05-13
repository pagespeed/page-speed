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

struct request_rec;

namespace html_rewriter {

// Forward declaration.
class HtmlRewriterImp;

class HtmlRewriter {
 public:
  explicit HtmlRewriter(request_rec* request,
                        const std::string& url, std::string* output);
  ~HtmlRewriter();
  // Rewrite input using internal StringWriter.
  void Rewrite(const char* input, int size);
  void Rewrite(const std::string& input) {
    Rewrite(input.data(), input.size());
  }

  // Flush the re-written content to output.
  void Flush();
  // Flush and finish the re-write.
  void Finish();

  const std::string& get_url() const;
  const std::string& set_url(const std::string& url);
 private:
  HtmlRewriterImp* html_rewriter_imp_;
};

}  // namespace html_rewriter

#endif  // HTML_REWRITER_HTML_REWRITER_H_
