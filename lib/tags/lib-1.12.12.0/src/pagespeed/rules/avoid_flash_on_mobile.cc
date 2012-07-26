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

#include "pagespeed/rules/avoid_flash_on_mobile.h"

#include "pagespeed/core/formatter.h"
#include "pagespeed/core/pagespeed_input.h"
#include "pagespeed/core/result_provider.h"
#include "pagespeed/core/rule_input.h"
#include "pagespeed/l10n/l10n.h"
#include "pagespeed/proto/pagespeed_output.pb.h"


namespace pagespeed {

namespace rules {

namespace {

const char* kRuleName = "AvoidFlashOnMobile";

}  // namespace

AvoidFlashOnMobile::AvoidFlashOnMobile()
    : pagespeed::Rule(pagespeed::InputCapabilities()) {}

const char* AvoidFlashOnMobile::name() const {
  return kRuleName;
}

UserFacingString AvoidFlashOnMobile::header() const {
  // TRANSLATOR: The name of a Page Speed rule that tells users to avoid using
  // Adobe Flash on mobile webpages
  return _("Avoid flash on mobile webpages");
}

bool AvoidFlashOnMobile::AppendResults(const RuleInput& rule_input,
                                       ResultProvider* provider) {
  const PagespeedInput& input = rule_input.pagespeed_input();
  for (int idx = 0, num = input.num_resources(); idx < num; ++idx) {
    const Resource& resource = input.GetResource(idx);
    if (resource.GetResourceType() == pagespeed::FLASH) {
      Result* result = provider->NewResult();
      result->add_resource_urls(resource.GetRequestUrl());
    }
  }
  return true;
}

void AvoidFlashOnMobile::FormatResults(const ResultVector& results,
                                       RuleFormatter* formatter) {
  if (results.empty()) {
    return;
  }

  UrlBlockFormatter* body = formatter->AddUrlBlock(
      // TRANSLATOR: Header at the top of a list of URLs of Adobe Flash
      // resources detected by Page Speed
      _("The following Flash objects are included on the page. "
          "Adobe Flash Player is not supported on Apple iOS or Android "
          "versions greater than 4.0.x+. Consider removing Flash objects and "
          "finding suitable replacements."));

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


int AvoidFlashOnMobile::ComputeScore(const InputInformation& input_info,
                                     const RuleResults& results) {
  // Scoring is binary: Flash == bad; no flash == good
  // TODO: This should be rethought if adapted for desktop usage.
  if (results.results_size() > 0) {
    return 0;
  }
  return 100;
}

bool AvoidFlashOnMobile::IsExperimental() const {
  return true;
}

}  // namespace rules

}  // namespace pagespeed
