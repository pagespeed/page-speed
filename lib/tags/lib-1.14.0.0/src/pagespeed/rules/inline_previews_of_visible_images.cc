// Copyright 2011 Google Inc. All Rights Reserved./
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

#include "pagespeed/rules/inline_previews_of_visible_images.h"

#include <algorithm>

#include "base/logging.h"
#include "pagespeed/core/dom.h"
#include "pagespeed/core/formatter.h"
#include "pagespeed/core/pagespeed_input.h"
#include "pagespeed/core/resource.h"
#include "pagespeed/core/result_provider.h"
#include "pagespeed/core/rule_input.h"
#include "pagespeed/dom/resource_coordinate_finder.h"
#include "pagespeed/l10n/l10n.h"
#include "pagespeed/proto/pagespeed_output.pb.h"

namespace pagespeed {

namespace rules {

InlinePreviewsOfVisibleImages::InlinePreviewsOfVisibleImages()
    : pagespeed::Rule(pagespeed::InputCapabilities(
        pagespeed::InputCapabilities::DOM |
        pagespeed::InputCapabilities::ONLOAD |
        pagespeed::InputCapabilities::REQUEST_START_TIMES)) {}

const char* InlinePreviewsOfVisibleImages::name() const {
  return "InlinePreviewsOfVisibleImages";
}

UserFacingString InlinePreviewsOfVisibleImages::header() const {
  // TRANSLATOR: Rule name. This rule recommends serving a low quality
  // version of the images that appear inside of the visible scroll
  // region of the browser (i.e. the part of the page that the user
  // can see without having to scroll down) inlined in the HTML
  // response.
  return _("Inline previews of visible images");
}

bool InlinePreviewsOfVisibleImages::AppendResults(const RuleInput& rule_input,
                                                  ResultProvider* provider) {
  const PagespeedInput& input = rule_input.pagespeed_input();
  std::vector<const pagespeed::Resource*> onscreen_resources;
  std::vector<const pagespeed::Resource*> offscreen_resources;
  if (!pagespeed::dom::FindOnAndOffscreenImageResources(
          input, &onscreen_resources, &offscreen_resources)) {
    return false;
  }

  for (std::vector<const pagespeed::Resource*>::const_iterator
           it = onscreen_resources.begin(), end = onscreen_resources.end();
       it != end; ++it) {
    const pagespeed::Resource& candidate = **it;
    if (candidate.GetResourceType() != pagespeed::IMAGE) {
      continue;
    }
    if (input.IsResourceLoadedAfterOnload(candidate)) {
      continue;
    }
    // TODO(bmcquade): look at the optimized image size here. If it
    // can be minified to under the threshold we should do that
    // instead.
    if (candidate.GetResponseBody().size() < kMinimumInlineThresholdBytes) {
      continue;
    }

    // TODO(bmcquade): what other data should we store here?
    Result* result = provider->NewResult();
    result->add_resource_urls(candidate.GetRequestUrl());
  }

  return true;
}

void InlinePreviewsOfVisibleImages::FormatResults(const ResultVector& results,
                                                  RuleFormatter* formatter) {
  if (results.empty()) {
    return;
  }

  UrlBlockFormatter* body = formatter->AddUrlBlock(
      // TRANSLATOR: Heading that gives a high-level overview of the
      // reason suggestions are being made.
      _("The following images are displayed within the initially visible "
        "region of the screen. To speed up rendering of the initially visible "
        "region of the page, inline a preview of these images and delay "
        "loading the full images until after page load is complete."));

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

bool InlinePreviewsOfVisibleImages::IsExperimental() const {
  // TODO(bmcquade): Before graduating from experimental:
  // 1. implement ComputeScore
  // 2. implement ComputeResultImpact
  return true;
}

// We only suggest inlining previews if the original image is greater
// than 20kB in size.
const size_t InlinePreviewsOfVisibleImages::kMinimumInlineThresholdBytes =
    20 * 1024;

}  // namespace rules

}  // namespace pagespeed
