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

#include "pagespeed/testing/formatted_results_test_converter.h"

#include "base/logging.h"
#include "pagespeed/core/string_util.h"
#include "pagespeed/proto/pagespeed_proto_formatter.pb.h"

using pagespeed::FormatString;
using pagespeed::FormattedResults;
using pagespeed::FormattedRuleResults;
using pagespeed::FormattedUrlBlockResults;
using pagespeed::FormattedUrlResult;

namespace pagespeed_testing {

bool FormattedResultsTestConverter::Convert(
    const FormattedResults& results, std::string* out) {
  return ConvertFormattedResults(results, out);
}

bool FormattedResultsTestConverter::ConvertFormattedResults(
    const FormattedResults& results, std::string* out) {
  if (!results.IsInitialized()) {
    LOG(ERROR) << "FormattedResults instance not fully initialized.";
    return false;
  }

  for (int i = 0, len = results.rule_results_size(); i < len; ++i) {
    if (!ConvertFormattedRuleResults(results.rule_results(i), out)) {
      return false;
    }
  }

  return true;
}

bool FormattedResultsTestConverter::ConvertFormattedRuleResults(
    const FormattedRuleResults& rule_results, std::string* out) {
  if (!rule_results.IsInitialized()) {
    LOG(ERROR) << "FormattedRuleResults instance not fully initialized.";
    return false;
  }

  for (int i = 0, len = rule_results.url_blocks_size(); i < len; ++i) {
    if (!ConvertFormattedUrlBlockResults(rule_results.url_blocks(i), out)) {
      return false;
    }
  }
  return true;
}

bool FormattedResultsTestConverter::ConvertFormattedUrlBlockResults(
    const FormattedUrlBlockResults& url_block_results, std::string* out) {
  if (!url_block_results.IsInitialized()) {
    LOG(ERROR) << "FormattedUrlBlockResults instance not fully initialized.";
    return false;
  }

  if (url_block_results.has_header()) {
    ConvertFormatString(url_block_results.header(), out);
    out->append("\n");
  }

  for (int i = 0, len = url_block_results.urls_size(); i < len; ++i) {
    if (!ConvertFormattedUrlResult(url_block_results.urls(i), out)) {
      return false;
    }
  }
  return true;
}

bool FormattedResultsTestConverter::ConvertFormattedUrlResult(
    const FormattedUrlResult& url_result, std::string* out) {
  if (!url_result.IsInitialized()) {
    LOG(ERROR) << "FormattedUrlResult instance not fully initialized.";
    return false;
  }

  out->append("  ");
  ConvertFormatString(url_result.result(), out);
  out->append("\n");

  for (int i = 0, len = url_result.details_size(); i < len; ++i) {
    out->append("    * ");
    ConvertFormatString(url_result.details(i), out);
    out->append("\n");
  }

  return true;
}

void FormattedResultsTestConverter::ConvertFormatString(
    const FormatString& format_string, std::string* out) {
  if (format_string.args_size() > 0) {
    std::vector<std::string> sub;
    for (int i = 0, len = format_string.args_size(); i < len; ++i) {
      sub.push_back(format_string.args(i).localized_value());
    }
    out->append(pagespeed::string_util::ReplaceStringPlaceholders(
        format_string.format(), sub, NULL));
  } else {
    out->append(format_string.format());
  }
}

}  // namespace pagespeed_testing
