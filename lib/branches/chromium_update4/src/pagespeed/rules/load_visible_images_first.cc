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

#include "pagespeed/rules/load_visible_images_first.h"

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

LoadVisibleImagesFirst::LoadVisibleImagesFirst()
    : pagespeed::Rule(pagespeed::InputCapabilities(
        pagespeed::InputCapabilities::DOM |
        pagespeed::InputCapabilities::ONLOAD |
        pagespeed::InputCapabilities::REQUEST_START_TIMES)) {}

const char* LoadVisibleImagesFirst::name() const {
  return "LoadVisibleImagesFirst";
}

UserFacingString LoadVisibleImagesFirst::header() const {
  // TRANSLATOR: Rule name. This rule recommends loading the images
  // that appear outside of the visible scroll region of the browser
  // (i.e. the part of the page that the user needs to scroll down to
  // see) after the images and resources needed to show the part
  // within the initially visible region have loaded.
  return _("Load visible images first");
}

bool LoadVisibleImagesFirst::AppendResults(const RuleInput& rule_input,
                                           ResultProvider* provider) {
  const PagespeedInput& input = rule_input.pagespeed_input();
  std::vector<const pagespeed::Resource*> onscreen_resources;
  std::vector<const pagespeed::Resource*> offscreen_resources;
  if (!pagespeed::dom::FindOnAndOffscreenImageResources(
          input, &onscreen_resources, &offscreen_resources)) {
    return false;
  }

  const pagespeed::Resource* last_requested_above_the_fold_resource = NULL;
  for (std::vector<const pagespeed::Resource*>::const_iterator
           it = onscreen_resources.begin(), end = onscreen_resources.end();
       it != end; ++it) {
    const pagespeed::Resource& resource = **it;
    if (resource.GetResourceType() != pagespeed::IMAGE) {
      continue;
    }
    if (input.IsResourceLoadedAfterOnload(resource)) {
      continue;
    }

    // TODO(bmcquade): Ideally we would track the last above the
    // fold resource to finish loading, and look for below-the-fold
    // images that started loading before then, but resource finish
    // times are currently not available in PagespeedInput. We'll
    // need to add them to improve the quality of this rule.
    if (last_requested_above_the_fold_resource == NULL ||
        last_requested_above_the_fold_resource->IsRequestStartTimeLessThan(
            resource)) {
      last_requested_above_the_fold_resource = &resource;
    }
  }

  if (last_requested_above_the_fold_resource == NULL) {
    // There are no above the fold image resources, so there is
    // nothing to prioritize relative to.
    return true;
  }

  for (std::vector<const pagespeed::Resource*>::const_iterator it =
           offscreen_resources.begin(), end = offscreen_resources.end();
       it != end; ++it) {
    const pagespeed::Resource& candidate = **it;
    if (candidate.GetResourceType() != pagespeed::IMAGE) {
      continue;
    }
    if (candidate.IsRequestStartTimeLessThan(
            *last_requested_above_the_fold_resource)) {
      // TODO(bmcquade): what other data should we store here?
      Result* result = provider->NewResult();
      result->add_resource_urls(candidate.GetRequestUrl());
    }
  }

  return true;
}

void LoadVisibleImagesFirst::FormatResults(const ResultVector& results,
                                           RuleFormatter* formatter) {
  if (results.empty()) {
    return;
  }

  UrlBlockFormatter* body = formatter->AddUrlBlock(
      // TRANSLATOR: Heading that gives a high-level overview of the
      // reason suggestions are being made.
      _("The following images are displayed outside of the initially visible "
        "region of the screen. Defer loading of these images to allow the "
        "initially visible region of the page to load faster."));

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

bool LoadVisibleImagesFirst::IsExperimental() const {
  // TODO(bmcquade): Before graduating from experimental:
  // 1. implement ComputeScore
  // 2. implement ComputeResultImpact
  return true;
}

}  // namespace rules

}  // namespace pagespeed
