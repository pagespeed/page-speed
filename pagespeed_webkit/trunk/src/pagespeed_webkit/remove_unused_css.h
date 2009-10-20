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

#ifndef PAGESPEED_WEBKIT_REMOVE_UNUSED_CSS_H_
#define PAGESPEED_WEBKIT_REMOVE_UNUSED_CSS_H_

#include <string>
#include <vector>
#include "base/basictypes.h"
#include "pagespeed/core/rule.h"

namespace pagespeed_webkit {

class CssStyleInfo;

class RemoveUnusedCSS : public pagespeed::Rule {
 public:
  // TODO PagespeedInput should provide styles info.
  RemoveUnusedCSS(const std::vector<CssStyleInfo*>& styles);

  // Rule interface.
  virtual bool AppendResults(const pagespeed::PagespeedInput& input,
                             pagespeed::Results* results);
  virtual void FormatResults(const std::vector<const pagespeed::Result*>& results,
                             pagespeed::Formatter* formatter);

 private:
  std::vector<CssStyleInfo*> styles_;
  DISALLOW_COPY_AND_ASSIGN(RemoveUnusedCSS);
};

}  // namespace pagespeed_webkit

#endif  // PAGESPEED_WEBKIT_REMOVE_UNUSED_CSS_H_
