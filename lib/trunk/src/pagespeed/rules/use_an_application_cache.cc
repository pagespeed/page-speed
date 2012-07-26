// Copyright 2011 Google Inc. All Rights Reserved.
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

#include "pagespeed/rules/use_an_application_cache.h"

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

class ManifestFilter : public net_instaweb::EmptyHtmlFilter {
 public:
  explicit ManifestFilter(net_instaweb::HtmlParse* html_parse);

  virtual void StartDocument();
  virtual void StartElement(net_instaweb::HtmlElement* element);
  virtual const char *Name() const {
    return "ManifestFilter";
  }

  const std::string& manifest_url() const;
  const bool has_html() const;

 private:
  std::string manifest_url_;
  bool has_html_;

  DISALLOW_COPY_AND_ASSIGN(ManifestFilter);
};

ManifestFilter::ManifestFilter(net_instaweb::HtmlParse* html_parse)
    : has_html_(false) {
}

void ManifestFilter::StartDocument() {
  // This is not usable for nested iframes, since we check only the primary
  // resource for manifest.
  has_html_ = false;
  manifest_url_.clear();
}

void ManifestFilter::StartElement(net_instaweb::HtmlElement* element) {
  net_instaweb::HtmlName::Keyword keyword = element->keyword();
  if (keyword == net_instaweb::HtmlName::kHtml) {
    has_html_ = true;
    const char* manifest_value =
        element->AttributeValue(net_instaweb::HtmlName::kManifest);
    if (manifest_value != NULL) {
      // Manifest exits.
      manifest_url_ = manifest_value;
      return;
    }
  }
}

const std::string& ManifestFilter::manifest_url() const {
  return manifest_url_;
}

const bool ManifestFilter::has_html() const {
  return has_html_;
}

}  // namespace

namespace rules {

namespace {

const char* kRuleName = "UseAnApplicationCache";

}  // namespace

UseAnApplicationCache::UseAnApplicationCache()
    : pagespeed::Rule(pagespeed::InputCapabilities(
        pagespeed::InputCapabilities::RESPONSE_BODY)) {
}

const char* UseAnApplicationCache::name() const {
  return kRuleName;
}

UserFacingString UseAnApplicationCache::header() const {
  // TRANSLATOR: The name of a Page Speed rule that tells users to make use
  // of the application cache to achieve faster startup times. This is displayed
  // in a list of rule names that Page Speed generates. "Application Cache"
  // should not be translated since it is the name of an HTML 5 feature.
  return _("Use an Application Cache");
}

bool UseAnApplicationCache::AppendResults(const RuleInput& rule_input,
                                   ResultProvider *provider) {
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
  ManifestFilter filter(&html_parse);
  html_parse.AddFilter(&filter);

  html_parse.StartParse(primary_resource_url);
  html_parse.ParseText(primary_resource->GetResponseBody().data(),
                       primary_resource->GetResponseBody().length());
  html_parse.FinishParse();

  if (filter.has_html() && filter.manifest_url().empty()) {
    // The primary resource has HTML tag, but not manifest attribute.
    Result *result = provider->NewResult();
    result->add_resource_urls(primary_resource_url);
    pagespeed::Savings* savings = result->mutable_savings();
    savings->set_requests_saved(1);
  }
  return true;
}

void UseAnApplicationCache::FormatResults(const ResultVector& results,
                                   RuleFormatter* formatter) {
  UserFacingString body_tmpl =
  // TRANSLATOR: Header at the top of a list of URLs that Page Speed detected
  // that HTML5 application cache should be used. It tells the user to fix the
  // problem by using application cache in the HTML documents.
  _("Using an application cache allows a page to show up immediately. The "
    "following HTML documents can use an application cache to reduce the time "
    "it takes for users to be able to interact with the page:");

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

bool UseAnApplicationCache::IsExperimental() const {
  return true;
}

}  // namespace rules

}  // namespace pagespeed
