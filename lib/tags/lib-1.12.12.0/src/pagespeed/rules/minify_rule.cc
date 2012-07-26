// Copyright 2009 Google Inc. All Rights Reserved.
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

#include "pagespeed/rules/minify_rule.h"

#include <algorithm>
#include <string>
#include <vector>

#include "base/logging.h"
#include "base/scoped_ptr.h"
#include "pagespeed/core/formatter.h"
#include "pagespeed/core/pagespeed_input.h"
#include "pagespeed/core/resource.h"
#include "pagespeed/core/resource_util.h"
#include "pagespeed/core/result_provider.h"
#include "pagespeed/core/rule_input.h"
#include "pagespeed/l10n/l10n.h"
#include "pagespeed/proto/pagespeed_output.pb.h"

namespace pagespeed {

namespace rules {

MinifierOutput::MinifierOutput(bool can_be_minified,
                               int plain_minified_size,
                               const std::string* minified_content,
                               const std::string& minified_content_mime_type)
    : can_be_minified_(can_be_minified),
      plain_minified_size_(plain_minified_size),
      minified_content_(minified_content),
      minified_content_mime_type_(minified_content_mime_type) {}

// static
MinifierOutput* MinifierOutput::CannotBeMinified() {
  return new MinifierOutput(false, -1, NULL, "");
}

// static
MinifierOutput* MinifierOutput::PlainMinifiedSize(int plain_minified_size) {
  return new MinifierOutput(true, plain_minified_size, NULL, "");
}

// static
MinifierOutput* MinifierOutput::DoNotSaveMinifiedContent(
    const std::string& minified_content) {
  return new MinifierOutput(true, minified_content.size(),
                            new std::string(minified_content), "");
}

// static
MinifierOutput* MinifierOutput::SaveMinifiedContent(
    const std::string& minified_content,
    const std::string& minified_content_mime_type) {
  DCHECK(!minified_content_mime_type.empty());
  return new MinifierOutput(true, minified_content.size(),
                            new std::string(minified_content),
                            minified_content_mime_type);
}

bool MinifierOutput::GetCompressedMinifiedSize(int* output) const {
  if (minified_content_ == NULL) {
    return false;
  }
  return resource_util::GetGzippedSize(*minified_content_, output);
}

Minifier::Minifier() {}

Minifier::~Minifier() {}

MinifyRule::MinifyRule(Minifier* minifier)
    : pagespeed::Rule(pagespeed::InputCapabilities(
        pagespeed::InputCapabilities::RESPONSE_BODY)),
      minifier_(minifier) {}

MinifyRule::~MinifyRule() {}

const char* MinifyRule::name() const {
  return minifier_->name();
}

UserFacingString MinifyRule::header() const {
  return minifier_->header_format();
}

bool MinifyRule::AppendResults(const RuleInput& rule_input,
                               ResultProvider* provider) {
  bool error = false;
  const PagespeedInput& input = rule_input.pagespeed_input();
  for (int idx = 0, num = input.num_resources(); idx < num; ++idx) {
    const Resource& resource = input.GetResource(idx);

    scoped_ptr<const MinifierOutput> output(
        minifier_->Minify(resource, rule_input));
    if (output == NULL) {
      error = true;
      continue;
    } else if (!output->can_be_minified()) {
      continue;
    }

    int bytes_saved = 0;
    int bytes_original = 0;
    bool is_post_gzip = false;
    if (resource_util::IsCompressedResource(resource)) {
      int new_size;
      if (rule_input.GetCompressedResponseBodySize(resource, &bytes_original) &&
          output->GetCompressedMinifiedSize(&new_size)) {
        bytes_saved = bytes_original - new_size;
        is_post_gzip = true;
      } else {
        LOG(ERROR) << "Unable to compare compressed sizes for "
                   << resource.GetRequestUrl();
        error = true;
        continue;
      }
    } else {
      bytes_original = resource.GetResponseBody().size();
      bytes_saved = bytes_original - output->plain_minified_size();
    }

    if (bytes_saved <= 0) {
      continue;
    }

    Result* result = provider->NewResult();
    result->set_original_response_bytes(bytes_original);
    result->add_resource_urls(resource.GetRequestUrl());

    Savings* savings = result->mutable_savings();
    savings->set_response_bytes_saved(bytes_saved);

    MinificationDetails* min_details =
      result->mutable_details()->MutableExtension(
          MinificationDetails::message_set_extension);
    min_details->set_savings_are_post_gzip(is_post_gzip);

    if (output->should_save_minified_content() &&
        !resource.IsResponseBodyModified()) {
      result->set_optimized_content(*output->minified_content());
      result->set_optimized_content_mime_type(
          output->minified_content_mime_type());
    }
  }

  return !error;
}

void MinifyRule::FormatResults(const ResultVector& results,
                               RuleFormatter* formatter) {
  int total_original_size = 0;
  int total_bytes_saved = 0;

  for (ResultVector::const_iterator iter = results.begin(),
           end = results.end();
       iter != end;
       ++iter) {
    const Result& result = **iter;
    total_original_size += result.original_response_bytes();
    const Savings& savings = result.savings();
    total_bytes_saved += savings.response_bytes_saved();
  }

  if (total_bytes_saved == 0) {
    return;
  }

  UrlBlockFormatter* body = formatter->AddUrlBlock(
      minifier_->body_format(), BytesArgument(total_bytes_saved),
      PercentageArgument(total_bytes_saved, total_original_size));

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

    // Support for computing savings after gzip compression was added
    // in Page Speed 1.12. Page Speed results computed from older
    // versions of Page Speed did not compute savings after gzip
    // compression. Thus the absence of a details field indicates that
    // the computed savings are not post gzip.
    bool savings_are_post_gzip = false;
    if (result.has_details()) {
      const ResultDetails& details = result.details();
      if (!details.HasExtension(
              MinificationDetails::message_set_extension)) {
        LOG(DFATAL) << "MinificationDetails missing.";
        continue;
      }
      const MinificationDetails& min_details =
          details.GetExtension(MinificationDetails::message_set_extension);
      savings_are_post_gzip = min_details.savings_are_post_gzip();
    }

    const int bytes_saved = result.savings().response_bytes_saved();
    const int original_size = result.original_response_bytes();

    UrlFormatter* url_result = body->AddUrlResult(
        savings_are_post_gzip ?
        minifier_->child_format_post_gzip() :
        minifier_->child_format(),
        UrlArgument(result.resource_urls(0)), BytesArgument(bytes_saved),
        PercentageArgument(bytes_saved, original_size));
    if (result.has_id() && result.has_optimized_content()) {
      url_result->SetAssociatedResultId(result.id());
    }
  }
}

CostBasedScoreComputer::CostBasedScoreComputer(int64 max_possible_cost)
    : max_possible_cost_(max_possible_cost) {
}

CostBasedScoreComputer::~CostBasedScoreComputer() {}

int CostBasedScoreComputer::ComputeScore() {
  if (max_possible_cost_ <= 0) {
    LOG(DFATAL) << "Invalid value for max_possible_cost: "
                << max_possible_cost_;
    return -1;
  }

  int score =  static_cast<int>(
      100 * (max_possible_cost_ - ComputeCost()) / max_possible_cost_);

  // Lower bound at zero. If a site's resources are very unoptimized
  // then the computed score could go below zero.
  return std::max(0, score);
}

WeightedCostBasedScoreComputer::WeightedCostBasedScoreComputer(
    const RuleResults* results,
    int64 max_possible_cost,
    double cost_weight)
    : CostBasedScoreComputer(max_possible_cost),
      results_(results),
      cost_weight_(cost_weight) {
}

int64 WeightedCostBasedScoreComputer::ComputeCost() {
  int64 total_cost = 0;
  for (int idx = 0, end = results_->results_size(); idx < end; ++idx) {
    const Result& result = results_->results(idx);
    if (result.has_savings()) {
      const Savings& savings = result.savings();
      total_cost += savings.response_bytes_saved();
    }
  }

  return static_cast<int64>(total_cost * cost_weight_);
}

}  // namespace rules

}  // namespace pagespeed
