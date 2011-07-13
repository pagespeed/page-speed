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
// Author: aoates@google.com (Andrew Oates)

#include "pagespeed/formatters/proto_formatter.h"

#include <string>

#include "base/logging.h"
#include "base/stl_util-inl.h"
#include "pagespeed/core/rule.h"
#include "pagespeed/l10n/localizer.h"
#include "pagespeed/proto/pagespeed_proto_formatter.pb.h"

using pagespeed::FormatString;
using pagespeed::FormattedResults;
using pagespeed::FormattedRuleResults;
using pagespeed::FormattedUrlBlockResults;
using pagespeed::FormattedUrlResult;
using pagespeed::l10n::Localizer;

namespace pagespeed {

namespace formatters {

namespace {

// Localizes str iff str.ShouldLocalize() == true, returning true on success.
bool MaybeLocalizeString(const Localizer* loc,
                         const UserFacingString& str, std::string* out) {
  if (!out) {
    LOG(DFATAL) << "out == NULL";
    return false;
  }

  if (str.ShouldLocalize()) {
    return loc->LocalizeString(std::string(str), out);
  } else {
    // ShouldLocalize() is false for string constants that are not appropriate
    // for translation (i.e. those marked with not_localized(...), such as "$1"
    // or "$1 ($2)"), so we pass them through as-is.
    *out = std::string(str);
    return true;
  }
}

// Fills in a FormatString proto from a format string and arguments.
// TODO(aoates): move this functionality into the Argument and FormatterParams
// classes, to provide l10n for all formatters that want it.
void FillFormatString(const Localizer* loc,
                      const UserFacingString& format_str,
                      const std::vector<const FormatArgument*>& arguments,
                      FormatString* out) {
  MaybeLocalizeString(loc, format_str, out->mutable_format());

  for (int index = 0, limit = arguments.size(); index < limit; ++index) {
    bool success = true;
    std::string localized;

    FormatArgument* format_arg = out->add_args();
    *format_arg = *arguments[index];

    switch (format_arg->type()) {
      case FormatArgument::INT_LITERAL:
        success = loc->LocalizeInt(format_arg->int_value(), &localized);
        break;
      case FormatArgument::BYTES:
        success = loc->LocalizeBytes(format_arg->int_value(), &localized);
        break;
      case FormatArgument::DURATION:
        success = loc->LocalizeTimeDuration(format_arg->int_value(),
                                            &localized);
        break;
      case FormatArgument::STRING_LITERAL:
        // Don't localize string arguments, since they're used for
        // "user-generated" content (such as hostnames and domains).
        localized = format_arg->string_value();
        break;
      case FormatArgument::URL:
        success = loc->LocalizeUrl(format_arg->string_value(), &localized);
        break;
      case FormatArgument::VERBATIM_STRING:
        // Don't localize pre string arguments, since they're
        // inherently not localizable (usually contain data or code of
        // some sort).
        localized = format_arg->string_value();
        break;
      case FormatArgument::PERCENTAGE:
        success = loc->LocalizePercentage(format_arg->int_value(), &localized);
        break;
      default:
        LOG(DFATAL) << "Unknown argument type " << format_arg->type();
        format_arg->set_type(FormatArgument::STRING_LITERAL);
        format_arg->set_string_value("?");
        localized = "?";
        break;
    }
    if (!success) {
      LOG(WARNING) << "warning: unable to localize argument $" << (index + 1)
                   << " (" << localized << ") in format string '"
                   << format_str;
    }
    format_arg->set_localized_value(localized);
  }
}

}  // namespace

ProtoFormatter::ProtoFormatter(const Localizer* localizer,
                               FormattedResults* results)
    : localizer_(localizer), results_(results) {
  DCHECK(localizer_);
  DCHECK(results_);
}

ProtoFormatter::~ProtoFormatter() {
  STLDeleteContainerPointers(rule_formatters_.begin(),
                             rule_formatters_.end());
}

RuleFormatter* ProtoFormatter::AddRule(const Rule& rule, int score,
                                       double impact) {
  FormattedRuleResults* rule_results = results_->add_rule_results();
  rule_results->set_rule_name(rule.name());
  rule_results->set_rule_score(score);
  rule_results->set_rule_impact(impact);
  if (rule.IsExperimental()) {
    rule_results->set_experimental(true);
  }

  if (!MaybeLocalizeString(localizer_,
                           rule.header(),
                           rule_results->mutable_localized_rule_name())) {
    LOG(ERROR) << "Unable to LocalizeString " << rule.header();
  }

  RuleFormatter* rule_formatter =
      new ProtoRuleFormatter(localizer_, rule_results);
  rule_formatters_.push_back(rule_formatter);
  return rule_formatter;
}

void ProtoFormatter::SetOverallScore(int score) {
  DCHECK(0 <= score && score <= 100);
  results_->set_score(score);
}

void ProtoFormatter::Finalize() {
  // Now for a superhack. If a ResultFilter is used, it may produce
  // rule results with no suggestions, or possibly an overall
  // formatted results with no suggestions. In those cases we need to
  // manually repair the impact and score values so the user is not
  // confused by a non-100 score with no suggestions.
  bool has_any_results = false;
  for (int i = 0; i < results_->rule_results_size(); ++i) {
    FormattedRuleResults* rule_results =
        results_->mutable_rule_results(i);
    if (rule_results->url_blocks_size() == 0) {
      rule_results->set_rule_score(100);
      rule_results->set_rule_impact(0.0);
    } else {
      has_any_results = true;
    }
  }
  if (!has_any_results && results_->has_score()) {
    results_->set_score(100);
  }
}

ProtoRuleFormatter::ProtoRuleFormatter(const Localizer* localizer,
                                       FormattedRuleResults* rule_results)
    : localizer_(localizer), rule_results_(rule_results) {
  DCHECK(localizer_);
  DCHECK(rule_results_);
}

ProtoRuleFormatter::~ProtoRuleFormatter() {
  STLDeleteContainerPointers(url_block_formatters_.begin(),
                             url_block_formatters_.end());
}

UrlBlockFormatter* ProtoRuleFormatter::AddUrlBlock(
    const UserFacingString& format_str,
    const std::vector<const FormatArgument*>& arguments) {
  FormattedUrlBlockResults* url_block_results =
      rule_results_->add_url_blocks();
  FillFormatString(localizer_, format_str, arguments,
                   url_block_results->mutable_header());
  UrlBlockFormatter* url_block_formatter =
      new ProtoUrlBlockFormatter(localizer_, url_block_results);
  url_block_formatters_.push_back(url_block_formatter);
  return url_block_formatter;
}

ProtoUrlBlockFormatter::ProtoUrlBlockFormatter(
    const Localizer* localizer,
    FormattedUrlBlockResults* url_block_results)
    : localizer_(localizer), url_block_results_(url_block_results) {
  DCHECK(localizer_);
  DCHECK(url_block_results_);
}

ProtoUrlBlockFormatter::~ProtoUrlBlockFormatter() {
  STLDeleteContainerPointers(url_formatters_.begin(),
                             url_formatters_.end());
}

UrlFormatter* ProtoUrlBlockFormatter::AddUrlResult(
    const UserFacingString& format_str,
    const std::vector<const FormatArgument*>& arguments) {
  FormattedUrlResult* url_result = url_block_results_->add_urls();
  FillFormatString(localizer_, format_str, arguments,
                   url_result->mutable_result());
  UrlFormatter* url_formatter =
      new ProtoUrlFormatter(localizer_, url_result);
  url_formatters_.push_back(url_formatter);
  return url_formatter;
}

ProtoUrlFormatter::ProtoUrlFormatter(const Localizer* localizer,
                                     FormattedUrlResult* url_result)
    : localizer_(localizer), url_result_(url_result) {
  DCHECK(localizer_);
  DCHECK(url_result_);
}

void ProtoUrlFormatter::AddDetail(
    const UserFacingString& format_str,
    const std::vector<const FormatArgument*>& arguments) {
  FillFormatString(localizer_, format_str, arguments,
                   url_result_->add_details());
}

void ProtoUrlFormatter::SetAssociatedResultId(int id) {
  url_result_->set_associated_result_id(id);
}

}  // namespace formatters

}  // namespace pagespeed
