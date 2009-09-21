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

#ifndef PAGESPEED_APPS_HTML_FORMATTER_H_
#define PAGESPEED_APPS_HTML_FORMATTER_H_

#include <iostream>

#include "pagespeed/core/formatter.h"

namespace pagespeed {

namespace html {

/**
 * Formatter that produces HTML.
 */
class HtmlFormatter : public Formatter {
 public:
  explicit HtmlFormatter(std::ostream* output);

 protected:
  // Formatter interface
  virtual Formatter* NewChild(const std::string& format_str,
                              const std::vector<const Argument*>& arguments);
  virtual void DoneAddingChildren();

 private:
  HtmlFormatter(std::ostream* output, int level);
  void Indent(int level);
  std::string Format(const std::string& format_str,
                     const std::vector<const Argument*>& arguments);
  std::ostream* output_;
  int level_;
  bool has_children_;
};

}  // namespace html

}  // namespace pagespeed

#endif  // PAGESPEED_APPS_HTML_FORMATTER_H_
