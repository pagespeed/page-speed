// Copyright 2009 Google Inc.
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

#ifndef PAGESPEED_WEBKIT_WEBKIT_CSS_STYLE_INFO_H_
#define PAGESPEED_WEBKIT_WEBKIT_CSS_STYLE_INFO_H_

#include <vector>

#include "pagespeed_webkit/css_style_info.h"

namespace WebCore {
class Page;
}

namespace pagespeed_webkit {

class WebkitCssStyleInfo : public CssStyleInfo {
 public:
  WebkitCssStyleInfo(const std::string& url,
                     const std::string& selector,
                     const std::string& css_text,
                     bool used) : url_(url),
                                  selector_(selector),
                                  css_text_(css_text),
                                  used_(used) {}
  virtual const std::string& url() const { return url_; }
  virtual const std::string& selector() const { return selector_; }
  virtual const std::string& css_text() const { return css_text_; }
  virtual bool used() const { return used_; }

 private:
  std::string url_;
  std::string selector_;
  std::string css_text_;
  bool used_;
  DISALLOW_COPY_AND_ASSIGN(WebkitCssStyleInfo);
};

void GatherCssStyles(WebCore::Page* page, std::vector<CssStyleInfo*>* styles);

}  // namespace pagespeed_webkit

#endif  // PAGESPEED_WEBKIT_WEBKIT_CSS_STYLE_INFO_H_
