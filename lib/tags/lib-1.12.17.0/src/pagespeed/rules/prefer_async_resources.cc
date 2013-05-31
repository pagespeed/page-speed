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

#include "pagespeed/rules/prefer_async_resources.h"

#include <string>
#include <vector>

#include "base/basictypes.h"
#include "base/logging.h"
#include "pagespeed/core/dom.h"
#include "pagespeed/core/formatter.h"
#include "pagespeed/core/pagespeed_input.h"
#include "pagespeed/core/resource.h"
#include "pagespeed/core/result_provider.h"
#include "pagespeed/core/rule_input.h"
#include "pagespeed/l10n/l10n.h"
#include "pagespeed/proto/pagespeed_output.pb.h"

namespace pagespeed {

namespace {

const char* kRuleName = "PreferAsyncResources";

const size_t kMaxChunksPerScriptMatcher = 5;

const char* kScriptMatchers[][kMaxChunksPerScriptMatcher] = {
  // Google Analytics. See:
  // https://developers.google.com/analytics/devguides/collection/gajs/asyncTracking
  { "google-analytics.com/ga.js", NULL },

  // Urchin.js is the name of the code snippet for an old version of the Google
  // Analytics tracking code.  We strongly recommend you to use the new Google
  // Analytics. See: http://www.google.com/urchin/faq.html
  { "google-analytics.com/urchin.js", NULL },

  // See: http://developers.facebook.com/docs/reference/javascript/
  { "connect.facebook.net/", "/all.js", NULL },

  // See: https://developers.google.com/+/web/+1button/#async-load
  { "apis.google.com/js/plusone.js", NULL },

  // See: https://twitter.com/about/resources/buttons
  { "platform.twitter.com/widgets.js", NULL },

  // Quantcast.  See:
  // https://www.quantcast.com/learning-center/guides/using-the-quantcast-asynchronous-tag/
  { "quantserve.com/quant.js", NULL },

  // comScore tag uses https://sb.scorecardresearch.com/ for secure site, and
  // http://b.scorecardresearch.com/ for HTTP.
  // See: http://www.netmonitor.cz/sites/default/files/mobilewebcensus-allplatforms.pdf
  { "b.scorecardresearch.com/beacon.js", NULL},

  // Google DFP GPT.  See:
  // https://support.google.com/dfp_premium/bin/answer.py?hl=en&answer=1638622
  { "www.googletagservices.com/tag/js/gpt.js", NULL},

  // ShareThis: http://support.sharethis.com/customer/portal/articles/475260
  { "w.sharethis.com/button/buttons.js", NULL},

  // Pinterest:
  // https://help.pinterest.com/entries/21101982-adding-the-pin-it-button-to-your-website
  { "assets.pinterest.com/js/pinit.js", NULL},

  // Disqus: http://disqus.com/admin/universalcode/
  { "disqus.com/", "count.js", NULL},
  // Seems few sites are using hte embed.js (total of 326 in 2013/03/13
  // httparchive).
  { "disqus.com/", "embed.js", NULL},

  // ChartBeat: http://chartbeat.com/docs/adding_the_code/
  { "static.chartbeat.com/js/chartbeat.js", NULL},

  // New Relic: https://newrelic.com/docs/features/how-does-real-user-monitoring-work#rum-for-browsers-without-the-navigation-timing-api
  { "d1ros97qkrwjf5.cloudfront.net/", "/eum/rum.js", NULL},

  // Clicky: no documentation found, but there is a blog post:
  // http://clicky.com/blog/205/asynchronous-tracking-code-take-2
  { "static.getclicky.com/js", NULL},

  // BuySellAds: http://blog.buysellads.com/2010/new-feature-non-blocking-asynchronous-ad-code/
  { "s3.buysellads.com/ac/bsa.js", NULL},

  // StumbleUpon: no documentation found.
  { "platform.stumbleupon.com/", "/widgets.js", NULL},

  // Yandex: no doc, or in russian that I cannot read.
  { "mc.yandex.ru/metrika/watch.js", NULL},

  // Tynt: no doc.
  { "cdn.tynt.com/tc.js", NULL},
  { "cdn.tynt.com/ti.js", NULL},

  // NOTE: Add additional scripts here that can be loaded asynchronously.
};

const size_t kNumScriptMatchers =
    sizeof(kScriptMatchers) / sizeof(kScriptMatchers[0]);

bool IsBlockingScript(
    const char** script_matcher, const std::string& resolved_src) {
  // strip query parameters from the source
  size_t query_params_pos = resolved_src.find_first_of('?');
  std::string stripped_resolved_src(resolved_src, 0, query_params_pos);

  size_t offset = 0;
  bool is_blocking_script = true;

  for (const char** script_match_chunk = &(script_matcher[0]);
       *script_match_chunk != NULL; ++script_match_chunk) {
    const size_t script_match_chunk_len = strlen(*script_match_chunk);
    const size_t remaining_resolved_src_len =
        stripped_resolved_src.size() - offset;
    if (script_match_chunk_len > remaining_resolved_src_len) {
      // The chunk is longer than the entire URL, so it can't
      // possibly match. Skip this matcher.
      is_blocking_script = false;
      break;
    }

    size_t pos = stripped_resolved_src.find(*script_match_chunk, offset);
    if (pos == std::string::npos) {
      // failed to find the chunk in the url, so skip the matcher.
      is_blocking_script = false;
      break;
    }

    offset = pos + script_match_chunk_len;
  }

  // check for trailing characters
  return is_blocking_script && (offset == stripped_resolved_src.size());
}

class ScriptVisitor : public pagespeed::DomElementVisitor {
 public:
  static void CheckDocument(const PagespeedInput* pagespeed_input,
                            const DomDocument* document,
                            ResultProvider* provider) {
    if (document) {
      ScriptVisitor visitor(pagespeed_input, document, provider);
      document->Traverse(&visitor);
      visitor.AddViolations(provider, document->GetDocumentUrl());
    }
  }

  virtual void Visit(const DomElement& node) {
    const std::string tag_name(node.GetTagName());
    if (tag_name == "IFRAME") {
      scoped_ptr<pagespeed::DomDocument> child_doc(node.GetContentDocument());
      CheckDocument(pagespeed_input_, child_doc.get(), provider_);
    } else if (pagespeed_input_->has_resource_with_url(
        document_->GetDocumentUrl())) {
      if (tag_name == "SCRIPT") {
        std::string script_src;
        if (node.GetAttributeByName("src", &script_src)) {
          std::string async;
          // The presence of a boolean attribute on an element represents
          // the true value.
          if (!node.GetAttributeByName("async", &async)) {
            VisitExternalScript(script_src);
          }
        }
      }
    }
  }

  void VisitExternalScript(const std::string& script_src);

  void AddViolations(ResultProvider* provider, const std::string& document_url);

 private:
  ScriptVisitor(const PagespeedInput* pagespeed_input,
                const DomDocument* document, ResultProvider* provider)
      : pagespeed_input_(pagespeed_input),
        document_(document), provider_(provider) {}

  std::vector<std::string> blocking_scripts_;

  const PagespeedInput* pagespeed_input_;
  const DomDocument* document_;
  ResultProvider* provider_;

  DISALLOW_COPY_AND_ASSIGN(ScriptVisitor);
};

void ScriptVisitor::VisitExternalScript(const std::string& script_src) {
  // Make sure to resolve the URI.
  std::string resolved_src = document_->ResolveUri(script_src);
  const pagespeed::Resource* resource =
      pagespeed_input_->GetResourceWithUrlOrNull(resolved_src);
  if (resource == NULL) {
    return;
  }
  if (pagespeed_input_->IsResourceLoadedAfterOnload(*resource)) {
    return;
  }

  for (size_t i = 0; i < kNumScriptMatchers; i++) {
    if (IsBlockingScript(kScriptMatchers[i], resolved_src)) {
      blocking_scripts_.push_back(resolved_src);
      break;
    }
  }
}

void ScriptVisitor::AddViolations(ResultProvider* provider,
                                  const std::string& document_url) {
  int index = 0;
  for (std::vector<std::string>::const_iterator
       iter = blocking_scripts_.begin(),
       end = blocking_scripts_.end();
       iter != end; ++index, ++iter) {
    Result* result = provider->NewResult();
    result->add_resource_urls(document_url);
    Savings* savings = result->mutable_savings();
    savings->set_critical_path_length_saved(1);
    ResultDetails* details = result->mutable_details();
    PreferAsyncResourcesDetails* async_details =
        details->MutableExtension(
            PreferAsyncResourcesDetails::message_set_extension);
    async_details->set_resource_url(*iter);
  }
}

}  // namespace

namespace rules {

PreferAsyncResources::PreferAsyncResources()
    : pagespeed::Rule(pagespeed::InputCapabilities(
        pagespeed::InputCapabilities::DOM |
        pagespeed::InputCapabilities::ONLOAD |
        pagespeed::InputCapabilities::REQUEST_START_TIMES)) {
}

const char* PreferAsyncResources::name() const {
  return kRuleName;
}

UserFacingString PreferAsyncResources::header() const {
  // TRANSLATOR: The name of a Page Speed rule that tells users to use
  // asynchronous resources. This is displayed in a list of rule names that
  // Page Speed generates.
  return _("Prefer asynchronous resources");
}

bool PreferAsyncResources::AppendResults(const RuleInput& rule_input,
                                         ResultProvider* provider) {
  const PagespeedInput& input = rule_input.pagespeed_input();
  ScriptVisitor::CheckDocument(&input, input.dom_document(), provider);
  return true;
}

void PreferAsyncResources::FormatResults(const ResultVector& results,
                                         RuleFormatter* formatter) {
  if (results.empty()) {
    return;
  }

  UrlBlockFormatter* body = formatter->AddUrlBlock(
      // TRANSLATOR: Header at the top of a list of URLs that Page Speed
      // detected as loaded synchronously. It describes the problem and tells
      // the user how to fix by loading them asynchronously.
      _("The following resources are loaded synchronously. Load them "
        "asynchronously to reduce blocking of page rendering."));

  // CheckDocument adds the results in post-order.

  for (ResultVector::const_iterator i = results.begin(), end = results.end();
       i != end; ++i) {
    const Result& result = **i;
    if (result.resource_urls_size() != 1) {
      LOG(DFATAL) << "Unexpected number of resource URLs.  Expected 1, Got "
                  << result.resource_urls_size() << ".";
      continue;
    }

    const ResultDetails& details = result.details();
    if (details.HasExtension(
            PreferAsyncResourcesDetails::message_set_extension)) {
      const PreferAsyncResourcesDetails& async_details = details.GetExtension(
          PreferAsyncResourcesDetails::message_set_extension);

      // TRANSLATOR: Detail for resource that loads synchronously. The "$1" will
      // be replaced by the document (HTML page, or a iframe) URL; the "$2" will
      // be replaced by the resource URL.
      body->AddUrlResult(_("$1 loads $2 synchronously."),
                         UrlArgument(result.resource_urls(0)),
                         UrlArgument(async_details.resource_url()));
    } else {
      LOG(DFATAL) << "Async details missing.";
    }
  }
}

}  // namespace rules

}  // namespace pagespeed
