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

#include "pagespeed/proto/results_to_json_converter.h"

#include "base/json/json_writer.h"
#include "base/logging.h"
#include "base/values.h"
#include "pagespeed/proto/pagespeed_output.pb.h"

namespace pagespeed {

namespace proto {

// static
bool ResultsToJsonConverter::Convert(
    const pagespeed::Results& results, std::string* out) {
  base::Value* root = ConvertResults(results);
  if (root == NULL) {
    return false;
  }
  base::JSONWriter::Write(root, out);
  delete root;
  return true;
}

base::Value* ResultsToJsonConverter::ConvertVersion(
    const pagespeed::Version& version) {
  if (!version.IsInitialized()) {
    LOG(ERROR) << "Version instance not fully initialized.";
    return NULL;
  }
  base::DictionaryValue* root = new base::DictionaryValue();
  root->SetInteger("major", version.major());
  root->SetInteger("minor", version.minor());
  root->SetBoolean("official_release", version.official_release());
  return root;
}

base::Value* ResultsToJsonConverter::ConvertSavings(
    const pagespeed::Savings& savings) {
  if (!savings.IsInitialized()) {
    LOG(ERROR) << "Savings instance not fully initialized.";
    return NULL;
  }
  base::DictionaryValue* root = new base::DictionaryValue();
  if (savings.has_dns_requests_saved()) {
    root->SetInteger("dns_requests_saved", savings.dns_requests_saved());
  }
  if (savings.has_requests_saved()) {
    root->SetInteger("requests_saved", savings.requests_saved());
  }
  if (savings.has_response_bytes_saved()) {
    root->SetInteger("response_bytes_saved", savings.response_bytes_saved());
  }
  if (savings.has_request_bytes_saved()) {
    root->SetInteger("request_bytes_saved", savings.request_bytes_saved());
  }
  if (savings.has_critical_path_length_saved()) {
    root->SetInteger("critical_path_length_saved",
                     savings.critical_path_length_saved());
  }
  if (savings.has_connections_saved()) {
    root->SetInteger("connections_saved", savings.connections_saved());
  }
  return root;
}

base::Value* ResultsToJsonConverter::ConvertResult(
    const pagespeed::Result& result) {
  if (!result.IsInitialized()) {
    LOG(ERROR) << "Result instance not fully initialized.";
    return NULL;
  }
  base::DictionaryValue* root = new base::DictionaryValue();
  if (result.has_savings()) {
    root->Set("savings", ConvertSavings(result.savings()));
  }
  if (result.resource_urls_size() > 0) {
    base::ListValue* resource_urls = new base::ListValue();
    for (int i = 0, len = result.resource_urls_size(); i < len; ++i) {
      resource_urls->Append(base::Value::CreateStringValue(result.resource_urls(i)));
    }
    root->Set("resource_urls", resource_urls);
  }

  // TODO: implement details serializers. Not supported for now.
  return root;
}

base::Value* ResultsToJsonConverter::ConvertRuleResult(
    const pagespeed::RuleResults& rule_results) {
  if (!rule_results.IsInitialized()) {
    LOG(ERROR) << "RuleResults instance not fully initialized.";
    return NULL;
  }
  base::DictionaryValue* root = new base::DictionaryValue();
  root->SetString("rule_name", rule_results.rule_name());
  if (rule_results.has_rule_score()) {
    root->SetInteger("rule_score", rule_results.rule_score());
  }
  if (rule_results.has_rule_impact()) {
    root->SetDouble("rule_impact", rule_results.rule_impact());
  }
  if (rule_results.results_size() > 0) {
    base::ListValue* results = new base::ListValue();
    for (int i = 0, len = rule_results.results_size(); i < len; ++i) {
      results->Append(ConvertResult(rule_results.results(i)));
    }
    root->Set("results", results);
  }
  return root;
}

base::Value* ResultsToJsonConverter::ConvertResults(
    const pagespeed::Results& results) {
  if (!results.IsInitialized()) {
    LOG(ERROR) << "Results instance not fully initialized.";
    return NULL;
  }
  base::DictionaryValue* root = new base::DictionaryValue();

  if (results.has_version()) {
    root->Set("version", ConvertVersion(results.version()));
  }

  if (results.rule_results_size() > 0) {
    base::ListValue* rule_results = new base::ListValue();
    for (int i = 0, len = results.rule_results_size(); i < len; ++i) {
      rule_results->Append(ConvertRuleResult(results.rule_results(i)));
    }
    root->Set("rule_results", rule_results);
  }

  if (results.has_score()) {
    root->SetInteger("score", results.score());
  }

  return root;
}

}  // namespace proto

}  // namespace pagespeed
