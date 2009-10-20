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

#include "pagespeed_webkit/webkit_css_style_info.h"

#include "config.h"

#include "CSSStyleRule.h"
#include "Document.h"
#include "Frame.h"
#include "NodeList.h"
#include "Page.h"
#include "PlatformString.h"
#include "StyleSheetList.h"
#include "TextEncoding.h"

namespace pagespeed_webkit {

static std::string ToAscii(const WebCore::String& string) {
  const WebCore::TextEncoding& ascii = WebCore::ASCIIEncoding();

  const WebCore::CString& encoded =
      ascii.encode(string.characters(),
                   string.length(),
                   WebCore::URLEncodedEntitiesForUnencodables);

  return std::string(encoded.data(), encoded.length());
}

void GatherCssStyles(WebCore::Page* page, std::vector<CssStyleInfo*>* styles) {
  WebCore::Document* document = page->mainFrame()->document();
  RefPtr<WebCore::StyleSheetList> style_sheets = document->styleSheets();
  for (size_t sheet_idx = 0; sheet_idx < style_sheets->length(); ++sheet_idx) {
    RefPtr<WebCore::StyleList> style_list = style_sheets->item(sheet_idx);
    std::string sheet_url =
        ToAscii(style_sheets->item(sheet_idx)->baseURL().string());
    for (size_t style_idx = 0; style_idx < style_list->length(); ++style_idx) {
      WebCore::StyleBase* item = style_list->item(style_idx);
      if (!item->isStyleRule()) {
        // Unexpected object type.  I'm not sure if this can happen.
        continue;
      }

      WebCore::CSSStyleRule* rule = static_cast<WebCore::CSSStyleRule*>(item);
      WebCore::String selector = rule->selectorText();
      WebCore::ExceptionCode ec = 0;
      RefPtr<WebCore::NodeList> selected =
          document->querySelectorAll(selector, ec);
      if (ec) {
        printf("Query selectors failed with code %d while processing %s\n",
               ec, ToAscii(selector).c_str());
        continue;
      }

      styles->push_back(new WebkitCssStyleInfo(sheet_url,
                                               ToAscii(selector),
                                               ToAscii(rule->cssText()),
                                               selected->length() != 0));
    }
  }
}

}  // namespace pagespeed_webkit
