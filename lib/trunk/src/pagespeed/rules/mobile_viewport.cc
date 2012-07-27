// Copyright 2012 Google Inc. All Rights Reserved.
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

#include "pagespeed/rules/mobile_viewport.h"

#include <string>
#include <vector>

#include "base/logging.h"
#include "build/build_config.h"
#include "net/instaweb/htmlparse/public/empty_html_filter.h"
#include "net/instaweb/htmlparse/public/html_parse.h"
#include "net/instaweb/util/public/google_message_handler.h"
#include "pagespeed/core/formatter.h"
#include "pagespeed/core/pagespeed_input.h"
#include "pagespeed/core/resource.h"
#include "pagespeed/core/resource_util.h"
#include "pagespeed/core/result_provider.h"
#include "pagespeed/core/rule_input.h"
#include "pagespeed/core/string_util.h"
#include "pagespeed/core/uri_util.h"
#include "pagespeed/l10n/l10n.h"
#include "pagespeed/proto/pagespeed_output.pb.h"

namespace pagespeed {

namespace {

const char* kViewportMetaName = "viewport";

class MetaViewportFilter : public net_instaweb::EmptyHtmlFilter {
 public:
  explicit MetaViewportFilter(net_instaweb::HtmlParse* html_parse);

  virtual void StartDocument();
  virtual void StartElement(net_instaweb::HtmlElement* element);
  virtual const char *Name() const {
    return "MetaViewportFilter";
  }

  const bool has_meta_viewport() const;

 private:
  bool has_meta_viewport_;

  DISALLOW_COPY_AND_ASSIGN(MetaViewportFilter);
};

MetaViewportFilter::MetaViewportFilter(net_instaweb::HtmlParse* html_parse)
    : has_meta_viewport_(false) {
}

void MetaViewportFilter::StartDocument() {
  // This is not usable for nested iframes, since we check only the primary
  // resource for meta tags.
  has_meta_viewport_ = false;
}

void MetaViewportFilter::StartElement(net_instaweb::HtmlElement* element) {
  if (has_meta_viewport_) {
    // Already found a tag.
    return;
  }

  net_instaweb::HtmlName::Keyword keyword = element->keyword();
  if (keyword != net_instaweb::HtmlName::kMeta) {
    return;
  }

  const char* meta_name_value =
      element->AttributeValue(net_instaweb::HtmlName::kName);
  if (meta_name_value == NULL ||
      !pagespeed::string_util::LowerCaseEqualsASCII(
          meta_name_value, kViewportMetaName)) {
    return;
  }

  const char* meta_content_value =
      element->AttributeValue(net_instaweb::HtmlName::kContent);
  if (meta_content_value != NULL) {
    // We currently do not check the contents of the value, just that the tag
    // is set. The assumption is that if a page's author has added a viewport,
    // they have thought through what is a reasonable value for the page.
    has_meta_viewport_ = true;
    return;
  }
}

const bool MetaViewportFilter::has_meta_viewport() const {
  return has_meta_viewport_;
}

}  // namespace

namespace rules {

namespace {

const char* kRuleName = "MobileViewport";

}  // namespace

MobileViewport::MobileViewport()
    : pagespeed::Rule(pagespeed::InputCapabilities(
        pagespeed::InputCapabilities::RESPONSE_BODY)) {
}

const char* MobileViewport::name() const {
  return kRuleName;
}

UserFacingString MobileViewport::header() const {
  // TRANSLATOR: The name of a Page Speed rule that tells users to specify
  // a viewport for mobile devices. This is displayed in a list of rule names
  //that Page Speed generates. "Viewport" is code and should not be translated.
  return _("Specify a viewport for mobile browsers");
}

bool MobileViewport::AppendResults(const RuleInput& rule_input,
                                   ResultProvider* provider) {
  const PagespeedInput& input = rule_input.pagespeed_input();
  const std::string& primary_resource_url_fragment =
      input.primary_resource_url();
  std::string primary_resource_url;
  if (!uri_util::GetUriWithoutFragment(primary_resource_url_fragment,
                                       &primary_resource_url)) {
    primary_resource_url = primary_resource_url_fragment;
  }
  if (primary_resource_url.empty()) {
    LOG(INFO) << "Primary resource URL was not set";
    return false;
  }
  const Resource* primary_resource =
      input.GetResourceWithUrlOrNull(primary_resource_url);
  if (primary_resource == NULL) {
    LOG(INFO) << "No resource for " << primary_resource_url;
    return false;
  }
  net_instaweb::GoogleMessageHandler message_handler;
  message_handler.set_min_message_type(net_instaweb::kError);
  net_instaweb::HtmlParse html_parse(&message_handler);
  MetaViewportFilter filter(&html_parse);
  html_parse.AddFilter(&filter);

  html_parse.StartParse(primary_resource_url);
  html_parse.ParseText(primary_resource->GetResponseBody().data(),
                       primary_resource->GetResponseBody().length());
  html_parse.FinishParse();

  if (!filter.has_meta_viewport()) {
    Result *result = provider->NewResult();
    result->add_resource_urls(primary_resource_url);
  }
  return true;
}

void MobileViewport::FormatResults(const ResultVector& results,
                                   RuleFormatter* formatter) {
  // TRANSLATOR: Header at the top of a list of URLs that Page Speed detected
  // that do not have a viewport specified. It tells the user to fix the
  // problem by adding a meta tag specifying a viewport to the HTML documents.
  UserFacingString body_tmpl =
  _("The following pages do not specify a viewport. Consider adding a meta "
    "tag specifying a viewport so mobile browsers can render the document at "
    "a usable size.");

  if (results.empty()) {
    return;
  }

  UrlBlockFormatter* body = formatter->AddUrlBlock(body_tmpl);
  for (ResultVector::const_iterator iter = results.begin(), end = results.end();
      iter != end; ++iter) {
    const Result& result = **iter;
    if (result.resource_urls_size() != 1) {
      LOG(DFATAL) << "Unexpected number of resource URLs. Expected 1, got "
                  << result.resource_urls_size() << ".";
      continue;
    }

    body->AddUrl(result.resource_urls(0));
  }
}

bool MobileViewport::IsExperimental() const {
  return true;
}

}  // namespace rules

}  // namespace pagespeed
