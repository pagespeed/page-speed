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

#include "pagespeed/rules/defer_parsing_javascript.h"

#include <algorithm>
#include <string>
#include <map>
#include <utility>

#include "base/basictypes.h"
#include "base/logging.h"
#include "net/instaweb/htmlparse/public/empty_html_filter.h"
#include "net/instaweb/htmlparse/public/html_parse.h"
#include "net/instaweb/util/public/google_message_handler.h"
#include "pagespeed/core/dom.h"
#include "pagespeed/core/formatter.h"
#include "pagespeed/core/pagespeed_input.h"
#include "pagespeed/core/resource.h"
#include "pagespeed/core/result_provider.h"
#include "pagespeed/core/rule_input.h"
#include "pagespeed/core/string_util.h"
#include "pagespeed/core/uri_util.h"
#include "pagespeed/l10n/l10n.h"
#include "pagespeed/js/js_minify.h"
#include "pagespeed/proto/pagespeed_output.pb.h"

namespace pagespeed {

namespace {

const char* kRuleName = "DeferParsingJavaScript";

// If you change this, also change it in the test.
// Note that minified jquery.mobile-1.0a3.min.js is 54.4KB.
const size_t kMaxBlockOfJavascript = 1024*40;

// JavaScriptBlock is used in JavaScriptFilter to store and track the size and
// URL of JavaScript code.
class JavaScriptBlock {
 public:
  JavaScriptBlock(const std::string& name, size_t size, bool is_inline)
      : name_(name), size_(size), is_inline_(is_inline) {}

  const std::string& name() const {return name_;}
  size_t size() const {return size_;}
  void set_size(size_t size) {size_ = size;}
  bool is_inline() const {return is_inline_;}

  private:
  std::string name_;  // Url to external javascript, or URL and inline block #.
  size_t size_;
  bool is_inline_;
};

class JavaScriptFilter : public net_instaweb::EmptyHtmlFilter {
 public:
  typedef std::map<std::string, JavaScriptBlock> UrlToJavaScriptBlockMap;

  JavaScriptFilter(net_instaweb::HtmlParse* html_parse,
                   const pagespeed::PagespeedInput* input)
    : html_parse_(html_parse),
      pagespeed_input_(input),
      total_size_(0) {}
  virtual ~JavaScriptFilter() {}

  virtual void StartDocument() {
    pending_javascript_blocks_.clear();
    problem_javascript_blocks_.clear();
  }
  virtual void StartElement(net_instaweb::HtmlElement* element);
  virtual void EndElement(net_instaweb::HtmlElement* element);
  virtual void Characters(net_instaweb::HtmlCharactersNode* characters);
  virtual const char* Name() const { return "JavaScriptFilter"; }

  const UrlToJavaScriptBlockMap& pending_javascript_blocks() const {
    return pending_javascript_blocks_;
  }

  const UrlToJavaScriptBlockMap& problem_javascript_blocks() const {
    return problem_javascript_blocks_;
  }
  size_t total_size() const { return total_size_; }

 private:
  void AddJavascriptBlock(
      const std::string& url, const std::string& content, bool is_inline);
  JavaScriptBlock* FindExistingBlockForUrl(const std::string& url);
  void FlushPendingJavascriptBlocks();
  net_instaweb::HtmlParse* html_parse_;
  UrlToJavaScriptBlockMap pending_javascript_blocks_;
  UrlToJavaScriptBlockMap problem_javascript_blocks_;
  const pagespeed::PagespeedInput* pagespeed_input_;
  size_t total_size_;

  DISALLOW_COPY_AND_ASSIGN(JavaScriptFilter);
};

void JavaScriptFilter::AddJavascriptBlock(
    const std::string& url, const std::string& content, bool is_inline) {
  std::string minified;
  int size = 0;
  bool did_minify = js::GetMinifiedStringCollapsedJsSize(content,
                                                         &size);
  if (!did_minify) {
    LOG(INFO) << "Minify JS failed. Original size is used.";
    size = content.size();
  }
  if (size == 0) {
    return;
  }

  JavaScriptBlock* existing_block = FindExistingBlockForUrl(url);
  if (existing_block == NULL) {
    // This is a new block, so add it to the list of pending blocks.
    pending_javascript_blocks_.insert(
        std::make_pair(url, JavaScriptBlock(url, size, is_inline)));
  } else if (is_inline) {
    // Increment the size of inline JS for the HTML resource that
    // contains the inline script.
    existing_block->set_size(existing_block->size() + size);
  } else {
    LOG(INFO) << "Duplicated JavaScript: " << url;
    // Do not count into the total size for now. It may confuse users when it
    // shows a total size of X, but the only listed Y size of JavaScript code.
    // TODO(lsong): Duplicated JavaScript may need parse twice and execute
    // twice, and browsers behave differently, e.g. for Chrome, second parse is
    // almost zero cost, but full parse for FireFox. Revisit this if situation
    // changes.
   return;
  }
  total_size_ += size;
}

JavaScriptBlock* JavaScriptFilter::FindExistingBlockForUrl(
    const std::string& url) {
  UrlToJavaScriptBlockMap::iterator it = pending_javascript_blocks_.find(url);
  if (it != pending_javascript_blocks_.end()) {
    return &it->second;
  }

  it = problem_javascript_blocks_.find(url);
  if (it != problem_javascript_blocks_.end()) {
    return &it->second;
  }

  // There is no existing block for this URL.
  return NULL;
}

void JavaScriptFilter::FlushPendingJavascriptBlocks() {
  for (JavaScriptFilter::UrlToJavaScriptBlockMap::const_iterator it =
       pending_javascript_blocks_.begin();
       it != pending_javascript_blocks_.end();
       ++it) {
    problem_javascript_blocks_.insert(std::make_pair(it->first, it->second));
  }

  pending_javascript_blocks_.clear();
}

void JavaScriptFilter::StartElement(net_instaweb::HtmlElement* element) {
  net_instaweb::HtmlName::Keyword keyword = element->keyword();
  if (keyword == net_instaweb::HtmlName::kScript) {
    const char* src = element->AttributeValue(net_instaweb::HtmlName::kSrc);
    if (src != NULL) {
      // Make sure to resolve the URI.
      std::string resolved_src;
      if (!uri_util::ResolveUriForDocumentWithUrl(
              src,
              pagespeed_input_->dom_document(),
              html_parse_->url(),
              &resolved_src)) {
        // We failed to resolve relative to the document, so try to
        // resolve relative to the document's URL. This will be
        // correct unless the document contains a <base> tag.
        resolved_src = uri_util::ResolveUri(src, html_parse_->url());
      }
      const pagespeed::Resource* resource =
          pagespeed_input_->GetResourceWithUrlOrNull(resolved_src);
      if (resource == NULL) {
        LOG(INFO) << "Resource not found: " << resolved_src;
        return;
      }
      const net_instaweb::HtmlElement::Attribute* async =
          element->FindAttribute(net_instaweb::HtmlName::kAsync);
      const net_instaweb::HtmlElement::Attribute* defer =
          element->FindAttribute(net_instaweb::HtmlName::kDefer);
      // The presence of a boolean attribute on an element represents the true
      // value, and the absence of the attribute represents the false value.
      // (ref: HTML5 spec).

      if (async == NULL && defer == NULL) {
        // Note that this leaves the block pending. The rule may still be OK if
        // this script tag occured at the bottom of the body.
        AddJavascriptBlock(resolved_src, resource->GetResponseBody(), false);
      }
    }
  } else {
    FlushPendingJavascriptBlocks();
  }
}

void JavaScriptFilter::Characters(
    net_instaweb::HtmlCharactersNode* characters) {
  net_instaweb::HtmlName::Keyword keyword =
      net_instaweb::HtmlName::kNotAKeyword;
  net_instaweb::HtmlElement* parent = characters->parent();
  if (parent != NULL) {
    keyword = parent->keyword();
  }

  // inline script
  if (keyword == net_instaweb::HtmlName::kScript) {
    AddJavascriptBlock(html_parse_->url(), characters->contents(), true);
  } else {
    // Whitespace at the end of a body does not cause flushing. Other characters
    // should, however. Note that comments are fed through a different callback
    // which is not overriden, thus they also do not cause flushing.
    const std::string& contents = characters->contents();
    if (!ContainsOnlyWhitespaceASCII(contents)) {
      FlushPendingJavascriptBlocks();
    }
  }
}

void JavaScriptFilter::EndElement(net_instaweb::HtmlElement* element) {
  net_instaweb::HtmlName::Keyword keyword = element->keyword();
  if (keyword != net_instaweb::HtmlName::kScript &&
      keyword != net_instaweb::HtmlName::kBody &&
      keyword != net_instaweb::HtmlName::kHtml) {
    FlushPendingJavascriptBlocks();
  }
}
/* Return true if result1 has a greater size of JavaScript code than result 2,
 * false otherwise. If either of them has no size info, the url is used.
 * */
bool CompareResults(const Result* result1, const Result* result2) {
  const ResultDetails& details1 = result1->details();
  const ResultDetails& details2 = result2->details();
  if (!details1.HasExtension(
          DeferParsingJavaScriptDetails::message_set_extension) ||
      !details2.HasExtension(
          DeferParsingJavaScriptDetails::message_set_extension)) {
    return result1->resource_urls(0) < result2->resource_urls(0);
  }
  const DeferParsingJavaScriptDetails& defer_details1 = details1.GetExtension(
      DeferParsingJavaScriptDetails::message_set_extension);
  const DeferParsingJavaScriptDetails& defer_details2 = details2.GetExtension(
      DeferParsingJavaScriptDetails::message_set_extension);

  return defer_details1.minified_javascript_size() >
      defer_details2.minified_javascript_size();
}

size_t GetMinifiedJavaScriptSize(const Result& result) {
  const ResultDetails& details = result.details();
  if (details.HasExtension(
          DeferParsingJavaScriptDetails::message_set_extension)) {
    const DeferParsingJavaScriptDetails& defer_details = details.GetExtension(
        DeferParsingJavaScriptDetails::message_set_extension);
    return defer_details.minified_javascript_size();
  }
  return 0;
}
size_t GetTotalJavaScriptSize(const ResultVector& results) {
  int total_javascript_size= 0;
  for (ResultVector::const_iterator iter = results.begin(),
           end = results.end();
       iter != end;
       ++iter) {
    total_javascript_size += GetMinifiedJavaScriptSize(**iter);
  }
  return total_javascript_size;
}

size_t GetTotalJavaScriptSize(const RuleResults& results) {
  int total_javascript_size= 0;
  for (int idx = 0, end = results.results_size(); idx < end; ++idx) {
    const Result& result = results.results(idx);
    total_javascript_size += GetMinifiedJavaScriptSize(result);
  }
  return total_javascript_size;
}

}  // namespace

namespace rules {

DeferParsingJavaScript::DeferParsingJavaScript()
    : pagespeed::Rule(pagespeed::InputCapabilities(
        pagespeed::InputCapabilities::RESPONSE_BODY)) {
}

const char* DeferParsingJavaScript::name() const {
  return kRuleName;
}

UserFacingString DeferParsingJavaScript::header() const {
  // TRANSLATOR: The name of a Page Speed rule that tells users to defer
  // parsing of large amount of JavaScript code. This is displayed in a list
  // of rule names that Page Speed generates.
  return _("Defer parsing of JavaScript");
}

bool DeferParsingJavaScript::AppendResults(const RuleInput& rule_input,
                                           ResultProvider* provider) {
  const PagespeedInput& input = rule_input.pagespeed_input();
  net_instaweb::GoogleMessageHandler message_handler;
  message_handler.set_min_message_type(net_instaweb::kError);
  net_instaweb::HtmlParse html_parse(&message_handler);
  JavaScriptFilter filter(&html_parse, &input);
  html_parse.AddFilter(&filter);

  for (int i = 0, num = input.num_resources(); i < num; ++i) {
    const Resource& resource = input.GetResource(i);
    if (input.IsResourceLoadedAfterOnload(resource)) {
      continue;
    }

    if (resource.GetResourceType() != HTML) {
      continue;
    }

    html_parse.StartParse(resource.GetRequestUrl().c_str());
    html_parse.ParseText(
        resource.GetResponseBody().data(), resource.GetResponseBody().length());
    html_parse.FinishParse();

    const JavaScriptFilter::UrlToJavaScriptBlockMap& problem_javascript_blocks =
        filter.problem_javascript_blocks();
    if (problem_javascript_blocks.empty()) {
      continue;
    }
    if (filter.total_size() < kMaxBlockOfJavascript) {
      continue;
    }

    for (JavaScriptFilter::UrlToJavaScriptBlockMap::const_iterator it =
         problem_javascript_blocks.begin();
         it != problem_javascript_blocks.end();
         ++it) {
      Result* result = provider->NewResult();
      result->add_resource_urls(it->first);
      ResultDetails* details = result->mutable_details();
      DeferParsingJavaScriptDetails* defer_details =
          details->MutableExtension(
              DeferParsingJavaScriptDetails::message_set_extension);
      defer_details->set_is_inline((it->second).is_inline());
      defer_details->set_minified_javascript_size((it->second).size());
    }
  }
  return true;
}

void DeferParsingJavaScript::FormatResults(const ResultVector& results,
                                         RuleFormatter* formatter) {
  if (results.empty()) {
    return;
  }

  size_t total_javascript_size = GetTotalJavaScriptSize(results);
  if (total_javascript_size == 0) {
    return;
  }

  UrlBlockFormatter* body = formatter->AddUrlBlock(
      // TRANSLATOR: Header at the top of a list of URLs that Page Speed
      // detected to have JavaScript code. It describes the problem and tells
      // the user how to fix by defering parsing the JavaScript code.
      _("$1 of JavaScript is parsed during initial page load. Defer parsing "
        "JavaScript to reduce blocking of page rendering."),
      BytesArgument(total_javascript_size));

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
            DeferParsingJavaScriptDetails::message_set_extension)) {
      const DeferParsingJavaScriptDetails& defer_details = details.GetExtension(
          DeferParsingJavaScriptDetails::message_set_extension);

      UserFacingString format_str;
      if (defer_details.is_inline()) {
        // TRANSLATOR: Page Speed result for a single resource that should
        // defer parsing its inline JavaScript. The "$1"  will be replaced
        // by the document (HTML page, or a iframe) URL; the "$2" will be
        // replaced by the size of its inline JavaScripts.
        format_str = _("$1 ($2 of inline JavaScript)");
      } else {
        format_str = not_localized("$1 ($2)");
      }
      body->AddUrlResult(
          format_str, UrlArgument(result.resource_urls(0)),
          BytesArgument(defer_details.minified_javascript_size()));
    } else {
      LOG(DFATAL) << "Defer parsing details missing.";
    }
  }
}

void DeferParsingJavaScript::SortResultsInPresentationOrder(
    ResultVector* rule_results) const {
  // Sort the results in the order of minified javascirpt size.
  std::stable_sort(rule_results->begin(),
                   rule_results->end(),
                   CompareResults);
}

// User linear interpolation to calculate the score base on the warning
// size/score, and critical size/score pairs.
int DeferParsingJavaScript::ComputeScore(
    const InputInformation& input_info, const RuleResults& results) {
  const size_t kPerfectThresholdSize = kMaxBlockOfJavascript;
  const size_t kCriticalThresholdSize = 1024*300;
  const double kPerfectScore = 100.0;
  const double kCriticalScore = 50.0;
  size_t total_javascript_size = GetTotalJavaScriptSize(results);
  double rate = (kCriticalScore - kPerfectScore) /
      (kCriticalThresholdSize - kPerfectThresholdSize);
  double offset = kPerfectScore - kPerfectThresholdSize * rate;
  int score = static_cast<int>(total_javascript_size * rate + offset);
  if (score < 0) {
    score = 0;
  } else if (score > 100) {
    score = 100;
  }
  return score;
}

double DeferParsingJavaScript::ComputeResultImpact(
    const InputInformation& input_info, const Result& result) {
  const DeferParsingJavaScriptDetails& details = result.details().GetExtension(
      DeferParsingJavaScriptDetails::message_set_extension);
  int minified_size = details.minified_javascript_size();
  if (minified_size < 0) {
    LOG(DFATAL) << "Invalid minified javascript size: " << minified_size;
    minified_size = 0;
  }
  const ClientCharacteristics& client = input_info.client_characteristics();
  return client.javascript_parse_weight() * minified_size;
}

}  // namespace rules

}  // namespace pagespeed
