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

#include "pagespeed/rules/avoid_charset_in_meta_tag.h"

#include <string>
#include "base/logging.h"
#include "base/string_util.h"
#include "net/instaweb/htmlparse/public/empty_html_filter.h"
#include "net/instaweb/htmlparse/public/html_parse.h"
#include "net/instaweb/util/public/google_message_handler.h"
#include "pagespeed/core/formatter.h"
#include "pagespeed/core/pagespeed_input.h"
#include "pagespeed/core/resource.h"
#include "pagespeed/core/resource_util.h"
#include "pagespeed/core/result_provider.h"
#include "pagespeed/core/rule_input.h"
#include "pagespeed/l10n/l10n.h"
#include "pagespeed/proto/pagespeed_output.pb.h"

namespace {

const char* kContentTypeHeaderName = "content-type";

// In IE8 this is the default character set.
const char* kDefaultCharset = "iso-8859-1";

const char* kCharset = "charset";

bool GetCharsetFromHeader(const std::string& header, std::string* out_charset) {
  pagespeed::resource_util::DirectiveMap directives;
  if (!pagespeed::resource_util::GetHeaderDirectives(header, &directives)) {
    return false;
  }

  pagespeed::resource_util::DirectiveMap::const_iterator charset_it =
      directives.find(kCharset);
  if (charset_it == directives.end()) {
    return false;
  }

  *out_charset = charset_it->second;
  return true;
}

// Parses an HTML resource looking for <meta> tags with a character
// set defined.
class MetaCharsetFilter : public net_instaweb::EmptyHtmlFilter {
 public:
  explicit MetaCharsetFilter();
  virtual ~MetaCharsetFilter();

  virtual void StartDocument();
  virtual void StartElement(net_instaweb::HtmlElement* element);
  virtual const char* Name() const { return "MetaCharsetFilter"; }

  const std::string& meta_charset_content() const {
    return meta_charset_content_;
  }

  int meta_charset_begin_line_number() const {
    return meta_charset_begin_line_number_;
  }

 private:
  std::string meta_charset_content_;
  int meta_charset_begin_line_number_;

  DISALLOW_COPY_AND_ASSIGN(MetaCharsetFilter);
};

MetaCharsetFilter::MetaCharsetFilter() : meta_charset_begin_line_number_(-1) {}
MetaCharsetFilter::~MetaCharsetFilter() {}

void MetaCharsetFilter::StartDocument() {
  meta_charset_content_.clear();
}

void MetaCharsetFilter::StartElement(net_instaweb::HtmlElement* element) {
  if (meta_charset_begin_line_number_ > 0) {
    // Already found a tag.
    return;
  }

  net_instaweb::HtmlName::Keyword keyword = element->keyword();
  if (keyword != net_instaweb::HtmlName::kMeta) {
    return;
  }

  // HTML5 allows the charset to be specified like so: <meta
  // charset="UTF-8" />.
  //
  // TODO(bmcquade): switch to element->AttributeValue() once HtmlName
  // includes a "charset" keyword.
  for (int i = 0; i < element->attribute_size(); ++i) {
    const net_instaweb::HtmlElement::Attribute& attr = element->attribute(i);
    if (base::strcasecmp(kCharset, attr.name_str()) == 0 &&
        attr.DecodedValueOrNull() != NULL) {
      meta_charset_content_ = attr.DecodedValueOrNull();
      meta_charset_begin_line_number_ = element->begin_line_number();
      return;
    }
  }

  // Traditionally charset was specified via http-equiv, so check for
  // that case next.
  const char* equiv_header_name =
      element->AttributeValue(net_instaweb::HtmlName::kHttpEquiv);
  if (equiv_header_name == NULL) {
    return;
  }

  if (base::strcasecmp(kContentTypeHeaderName, equiv_header_name) != 0) {
    return;
  }

  const char* equiv_header_value =
      element->AttributeValue(net_instaweb::HtmlName::kContent);
  if (equiv_header_value == NULL) {
    return;
  }

  if (!GetCharsetFromHeader(equiv_header_value, &meta_charset_content_)) {
    return;
  }

  meta_charset_begin_line_number_ = element->begin_line_number();
}

}  // namespace

namespace pagespeed {

namespace rules {

AvoidCharsetInMetaTag::AvoidCharsetInMetaTag()
    : pagespeed::Rule(pagespeed::InputCapabilities(
        pagespeed::InputCapabilities::RESPONSE_BODY)) {
}

const char* AvoidCharsetInMetaTag::name() const {
  return "AvoidCharsetInMetaTag";
}

UserFacingString AvoidCharsetInMetaTag::header() const {
  // TRANSLATOR: The name of a Page Speed rule that tells users to ensure that
  // their webpages do not specify a character set in an HTML meta tag.
  // This is displayed in a list of rule names that Page Speed generates.
  return _("Avoid a character set in the meta tag");
}

bool AvoidCharsetInMetaTag::AppendResults(const RuleInput& rule_input,
                                          ResultProvider* provider) {
  const PagespeedInput& input = rule_input.pagespeed_input();

  for (int idx = 0, num = input.num_resources(); idx < num; ++idx) {
    const Resource& resource = input.GetResource(idx);
    const ResourceType resource_type = resource.GetResourceType();
    const std::string& content_type =
        resource.GetResponseHeader("Content-Type");

    if (resource_type != HTML) {
      const bool might_be_html = resource_type == OTHER && content_type.empty();
      if (!might_be_html) {
        // This rule only applies to HTML resources. However, if the
        // Content-Type header is not specified, it might be an HTML
        // resource that's missing a Content-Type, so include it in
        // the evaluation.
        continue;
      }
    }

    std::string charset;
    if (GetCharsetFromHeader(content_type, &charset)) {
      // There is a valid charset in the Content-Type header, so don't
      // flag this resource.
      continue;
    }

    std::string meta_charset_content;
    int meta_charset_begin_line_number;
    if (!HasMetaCharsetTag(resource.GetRequestUrl(),
                           resource.GetResponseBody(),
                           &meta_charset_content,
                           &meta_charset_begin_line_number)) {
      continue;
    }

    if (base::strcasecmp(kDefaultCharset, meta_charset_content.c_str()) == 0) {
      // If the user specified the default charset, the IE8 browser
      // will not disable the speculative parser, so don't warn.
      continue;
    }

    // There was a charset found in a meta tag.
    Result* result = provider->NewResult();

    // TODO(bmcquade): include a more precise estimate of savings once
    // we understand critical paths. For now, we assume we save one
    // critical path on IE8.
    Savings* savings = result->mutable_savings();
    savings->set_critical_path_length_saved(1);

    result->add_resource_urls(resource.GetRequestUrl());

    // TODO(bmcquade): add the tag line number and contents in a proto
    // extension
  }

  return true;
}

void AvoidCharsetInMetaTag::FormatResults(const ResultVector& results,
                                          RuleFormatter* formatter) {
  if (results.empty()) {
    return;
  }

  UrlBlockFormatter* body = formatter->AddUrlBlock(
      // TRANSLATOR: Header at the top of a list of URLs that Page
      // Speed detected as declaring the character set (e.g. UTF-8,
      // Latin-1, or some other text encoding) in an HTML meta tag. It
      // describes the problem to the user, and tells them how to fix
      // it by explicitly specifying the character set in the HTTP
      // Content-Type response header.
      _("The following resources have a character set specified in a meta tag. "
        "Specifying a character set in a meta tag disables the lookahead "
        "downloader in IE8. To improve resource download parallelization, move "
        "the character set to the HTTP Content-Type response header."));

  for (ResultVector::const_iterator iter = results.begin(),
           end = results.end();
       iter != end;
       ++iter) {
    const Result& result = **iter;
    if (result.resource_urls_size() != 1) {
      LOG(DFATAL) << "Unexpected number of resource URLs.  Expected 1, Got "
                  << result.resource_urls_size() << ".";
      continue;
    }
    body->AddUrl(result.resource_urls(0));
  }
}

/* static */
bool AvoidCharsetInMetaTag::HasMetaCharsetTag(
    const std::string& url,
    const std::string& html_body,
    std::string* out_meta_charset_content,
    int* out_meta_charset_begin_line_number) {
  net_instaweb::GoogleMessageHandler message_handler;
  message_handler.set_min_message_type(net_instaweb::kError);
  net_instaweb::HtmlParse html_parse(&message_handler);
  MetaCharsetFilter filter;
  html_parse.AddFilter(&filter);

  html_parse.StartParse(url.c_str());
  html_parse.ParseText(html_body.c_str(), html_body.size());
  html_parse.FinishParse();

  *out_meta_charset_begin_line_number = filter.meta_charset_begin_line_number();
  *out_meta_charset_content = filter.meta_charset_content();
  return *out_meta_charset_begin_line_number > 0;
}

bool AvoidCharsetInMetaTag::IsExperimental() const {
  // TODO(bmcquade): Before graduating from experimental:
  // 1. implement ComputeScore
  // 2. implement ComputeResultImpact
  return true;
}

}  // namespace rules

}  // namespace pagespeed
