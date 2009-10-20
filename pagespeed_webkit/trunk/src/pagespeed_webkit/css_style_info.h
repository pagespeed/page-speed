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

// Interface that provides access to CSS Style information.

#ifndef PAGESPEED_WEBKIT_CSS_STYLE_INFO_H_
#define PAGESPEED_WEBKIT_CSS_STYLE_INFO_H_

#include <string>
#include "base/basictypes.h"

namespace pagespeed_webkit {

class CssStyleInfo {
 public:
  CssStyleInfo() {}
  virtual ~CssStyleInfo() {}
  virtual const std::string& url() const = 0;
  virtual const std::string& selector() const = 0;
  virtual const std::string& css_text() const = 0;
  virtual bool used() const = 0;
 private:
  DISALLOW_COPY_AND_ASSIGN(CssStyleInfo);
};

}  // namespace pagespeed_webkit

#endif  // PAGESPEED_WEBKIT_CSS_STYLE_INFO_H_
