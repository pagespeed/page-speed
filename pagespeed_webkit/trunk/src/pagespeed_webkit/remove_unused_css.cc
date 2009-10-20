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

#include "pagespeed_webkit/remove_unused_css.h"

#include "base/logging.h"
#include "pagespeed/core/formatter.h"
#include "pagespeed/proto/pagespeed_output.pb.h"
#include "pagespeed_webkit/css_style_info.h"

namespace pagespeed_webkit {

RemoveUnusedCSS::RemoveUnusedCSS(const std::vector<CssStyleInfo*>& styles)
    : pagespeed::Rule("RemoveUnusedCSS"),
      styles_(styles) {
}

bool RemoveUnusedCSS::AppendResults(const pagespeed::PagespeedInput& input,
                                    pagespeed::Results* results) {
  typedef std::map<std::string, int> UrlSavingsMap;
  UrlSavingsMap url_savings_map;
  for (std::vector<CssStyleInfo*>::const_iterator iter = styles_.begin(),
           end = styles_.end();
       iter != end;
       ++iter) {
    CssStyleInfo* css_info = *iter;
    if (!css_info->used()) {
      url_savings_map[css_info->url()] += css_info->css_text().length();
    }
  }

  for (UrlSavingsMap::const_iterator iter = url_savings_map.begin(),
           end = url_savings_map.end();
       iter != end;
       ++iter) {
    pagespeed::Result* result = results->add_results();
    result->set_rule_name(name());

    pagespeed::Savings* savings = result->mutable_savings();
    savings->set_response_bytes_saved(iter->second);

    result->add_resource_urls(iter->first);
  }
}

void RemoveUnusedCSS::FormatResults(
    const std::vector<const pagespeed::Result*>& results,
    pagespeed::Formatter* formatter) {

  pagespeed::Formatter* header = formatter->AddChild("Remove unused CSS");

  for (std::vector<const pagespeed::Result*>::const_iterator
           iter = results.begin(), end = results.end();
       iter != end;
       ++iter) {
    const pagespeed::Result& result = **iter;
    const pagespeed::Savings& savings = result.savings();

    pagespeed::Argument bytes(pagespeed::Argument::BYTES,
                              savings.response_bytes_saved());
    pagespeed::Formatter* body = header->AddChild(
        "~$1 of CSS is not used by the current page.",
        bytes);

    CHECK(result.resource_urls_size() == 1);
    pagespeed::Argument url(pagespeed::Argument::URL, result.resource_urls(0));
    body->AddChild("$1", url);
  }
}

}  // namespace pagespeed_webkit
