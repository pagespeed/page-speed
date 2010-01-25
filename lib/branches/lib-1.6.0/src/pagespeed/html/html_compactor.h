// Copyright 2008 Google Inc. All Rights Reserved.
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

// This class takes an HTML document as input and produces another document as
// output that is equivalent but hopefully smaller in size. It handles ASCII or
// UTF-8 encoded HTML but not double-byte character set. The class is not
// thread-safe, but each instance can be reused for multiple documents.

#ifndef PAGESPEED_HTML_HTML_COMPACTOR_H_
#define PAGESPEED_HTML_HTML_COMPACTOR_H_

#include <string>

#include "base/basictypes.h"
#include "pagespeed/html/html_tag.h"

namespace pagespeed {

namespace html {

class HtmlCompactor {
 public:
  // Append a compacted version of input to output, and return true on success
  // or false on failure.
  static bool CompactHtml(const std::string& input, std::string* output);

 private:
  explicit HtmlCompactor(std::string *output);
  ~HtmlCompactor();

  bool CompactHtml(const std::string& input);
  const char *ProcessTag(const char* tag_begin,
                         const char* tag_end,
                         const char* all_end);
  void CompactForeign(const char* begin, const char* end);

  std::string* output_;  // current output string
  HtmlTag cur_tag_;  // current parsed tag

  DISALLOW_COPY_AND_ASSIGN(HtmlCompactor);
};

}  // namespace html

}  // namespace pagespeed

#endif  // PAGESPEED_HTML_HTML_COMPACTOR_H_
