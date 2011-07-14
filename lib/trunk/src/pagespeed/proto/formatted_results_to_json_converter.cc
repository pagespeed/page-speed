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

#include "pagespeed/proto/formatted_results_to_json_converter.h"

#include "base/json/json_writer.h"
#include "base/logging.h"
#include "base/values.h"
#include "base/scoped_ptr.h"
#include "pagespeed/proto/pagespeed_proto_formatter.pb.h"

namespace {

// This table must be kept in sync with the enum in
// pagespeed_proto_formatter.proto.
static const char* kArgumentTypeToNameMap[] = {
  "invalid",
  "url",
  "string",
  "int",
  "bytes",
  "duration",
  "verbatim",
  "percentage",
};

static const char* kInvalidArgumentType = kArgumentTypeToNameMap[0];

static const size_t kArgumentTypeToNameMapLen =
    sizeof(kArgumentTypeToNameMap) / sizeof(kArgumentTypeToNameMap[0]);

// Catch cases where we add or remove an entry to the FormatArgument
// enum but don't sync the kArgumentTypeToNameMap.
COMPILE_ASSERT(
    kArgumentTypeToNameMapLen ==
    static_cast<size_t>(
        pagespeed::FormatArgument_ArgumentType_ArgumentType_ARRAYSIZE),
    compile_assert_incomplete_argument_type_to_name_map);

}  // namespace

namespace pagespeed {

namespace proto {

bool FormattedResultsToJsonConverter::Convert(
    const pagespeed::FormattedResults& results, std::string* out) {
  scoped_ptr<Value> root(ConvertFormattedResults(results));
  if (root == NULL) {
    return false;
  }
  base::JSONWriter::Write(root.get(), false, out);
  return true;
}

Value* FormattedResultsToJsonConverter::ConvertFormattedResults(
    const pagespeed::FormattedResults& results) {
  if (!results.IsInitialized()) {
    LOG(ERROR) << "FormattedResults instance not fully initialized.";
    return NULL;
  }
  DictionaryValue* root = new DictionaryValue();
  root->SetString("locale", results.locale());
  if (results.has_score()) {
    root->SetInteger("score", results.score());
  }
  if (results.rule_results_size() > 0) {
    ListValue* rule_results = new ListValue();
    for (int i = 0, len = results.rule_results_size(); i < len; ++i) {
      rule_results->Append(
          ConvertFormattedRuleResults(results.rule_results(i)));
    }
    root->Set("rule_results", rule_results);
  }
  return root;
}

Value* FormattedResultsToJsonConverter::ConvertFormattedRuleResults(
    const pagespeed::FormattedRuleResults& rule_results) {
  if (!rule_results.IsInitialized()) {
    LOG(ERROR) << "FormattedRuleResults instance not fully initialized.";
    return NULL;
  }
  DictionaryValue* root = new DictionaryValue();
  root->SetString("rule_name", rule_results.rule_name());
  if (rule_results.has_rule_score()) {
    root->SetInteger("rule_score", rule_results.rule_score());
  }
  if (rule_results.has_rule_impact()) {
    root->SetDouble("rule_impact", rule_results.rule_impact());
  }
  if (rule_results.has_experimental()) {
    root->SetBoolean("experimental", rule_results.experimental());
  }
  root->SetString("localized_rule_name", rule_results.localized_rule_name());
  if (rule_results.url_blocks_size() > 0) {
    ListValue* url_blocks = new ListValue();
    for (int i = 0, len = rule_results.url_blocks_size(); i < len; ++i) {
      url_blocks->Append(
          ConvertFormattedUrlBlockResults(rule_results.url_blocks(i)));
    }
    root->Set("url_blocks", url_blocks);
  }
  return root;
}

Value* FormattedResultsToJsonConverter::ConvertFormattedUrlBlockResults(
    const pagespeed::FormattedUrlBlockResults& url_block_results) {
  if (!url_block_results.IsInitialized()) {
    LOG(ERROR) << "FormattedUrlBlockResults instance not fully initialized.";
    return NULL;
  }
  DictionaryValue* root = new DictionaryValue();
  if (url_block_results.has_header()) {
    root->Set("header", ConvertFormatString(url_block_results.header()));
  }

  if (url_block_results.urls_size() > 0) {
    ListValue* urls = new ListValue();
    for (int i = 0, len = url_block_results.urls_size(); i < len; ++i) {
      urls->Append(ConvertFormattedUrlResult(url_block_results.urls(i)));
    }
    root->Set("urls", urls);
  }
  if (url_block_results.has_associated_result_id()) {
    root->SetInteger("associated_result_id",
                     url_block_results.associated_result_id());
  }
  return root;
}

Value* FormattedResultsToJsonConverter::ConvertFormattedUrlResult(
    const pagespeed::FormattedUrlResult& url_result) {
  if (!url_result.IsInitialized()) {
    LOG(ERROR) << "FormattedUrlResult instance not fully initialized.";
    return NULL;
  }
  DictionaryValue* root = new DictionaryValue();
  root->Set("result", ConvertFormatString(url_result.result()));
  if (url_result.details_size() > 0) {
    ListValue* details = new ListValue();
    for (int i = 0, len = url_result.details_size(); i < len; ++i) {
      details->Append(ConvertFormatString(url_result.details(i)));
    }
    root->Set("details", details);
  }
  if (url_result.has_associated_result_id()) {
    root->SetInteger("associated_result_id",
                     url_result.associated_result_id());
  }
  return root;
}

Value* FormattedResultsToJsonConverter::ConvertFormatString(
    const pagespeed::FormatString& format_string) {
  if (!format_string.IsInitialized()) {
    LOG(ERROR) << "FormatString instance not fully initialized.";
    return NULL;
  }
  DictionaryValue* root = new DictionaryValue();
  root->SetString("format", format_string.format());
  if (format_string.args_size() > 0) {
    ListValue* args = new ListValue();
    for (int i = 0, len = format_string.args_size(); i < len; ++i) {
      args->Append(ConvertFormatArgument(format_string.args(i)));
    }
    root->Set("args", args);
  }

  return root;
}

Value* FormattedResultsToJsonConverter::ConvertFormatArgument(
    const pagespeed::FormatArgument& format_arg) {
  if (!format_arg.IsInitialized()) {
    LOG(ERROR) << "FormatArgument instance not fully initialized.";
    return NULL;
  }
  DictionaryValue* root = new DictionaryValue();
  root->SetString("type", ConvertFormatArgumentType(format_arg.type()));
  root->SetString("localized_value", format_arg.localized_value());
  if (format_arg.has_string_value()) {
    root->SetString("string_value", format_arg.string_value());
  }
  if (format_arg.has_int_value()) {
    root->SetInteger("int_value", static_cast<int>(format_arg.int_value()));
  }
  return root;
}

const char* FormattedResultsToJsonConverter::ConvertFormatArgumentType(
    int argument_type) {
  if (pagespeed::FormatArgument_ArgumentType_IsValid(argument_type)) {
    return kArgumentTypeToNameMap[argument_type];
  } else {
    return kInvalidArgumentType;
  }
}

}  // namespace proto

}  // namespace pagespeed
