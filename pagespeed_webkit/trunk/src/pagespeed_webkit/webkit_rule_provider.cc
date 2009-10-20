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

#include "webkit_rule_provider.h"

#include "remove_unused_css.cc"
#include "webkit_css_style_info.h"

namespace pagespeed_webkit {

namespace rule_provider {

void AppendWebkitRules(WebCore::Page* page,
                       std::vector<pagespeed::Rule*>* rules) {
  std::vector<pagespeed_webkit::CssStyleInfo*> styles;
  GatherCssStyles(page, &styles);
  rules->push_back(new RemoveUnusedCSS(styles));
}

}  // namespace rule_provider

}  // namespace pagespeed
