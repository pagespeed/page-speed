// Copyright 2010 Google Inc. All Rights Reserved.
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

#include "pagespeed/html/minify_js_css_filter.h"

#include <functional>
#include <string>

#include "base/logging.h"
#include "base/string_util.h"
#include "net/instaweb/htmlparse/public/html_parse.h"
#include "pagespeed/css/cssmin.h"
#include "pagespeed/js/js_minify.h"

namespace pagespeed {

namespace html {

MinifyJsCssFilter::MinifyJsCssFilter(net_instaweb::HtmlParse* html_parse)
    : html_parse_(html_parse) {
}

void MinifyJsCssFilter::Characters(
    net_instaweb::HtmlCharactersNode* characters) {
  net_instaweb::HtmlElement* parent = characters->parent();
  if (parent != NULL) {
    net_instaweb::HtmlName::Keyword keyword = parent->keyword();
    bool did_minify = false;
    std::string minified;
    if (keyword == net_instaweb::HtmlName::kScript) {
      did_minify = js::MinifyJs(characters->contents(), &minified);
      if (!did_minify) {
        LOG(INFO) << "Inline JS minification failed.";
      }
    } else if (keyword == net_instaweb::HtmlName::kStyle) {
      // We do not currently strip SGML comments from CSS since CSS
      // parsing behavior within CSS comments is inconsistent between
      // browsers.
      did_minify = css::MinifyCss(characters->contents(), &minified);
      if (!did_minify) {
        LOG(INFO) << "Inline CSS minification failed.";
      }
    }
    if (did_minify) {
      html_parse_->ReplaceNode(
          characters,
          html_parse_->NewCharactersNode(characters->parent(), minified));
    }
  }
}

}  // namespace html

}  // namespace pagespeed
