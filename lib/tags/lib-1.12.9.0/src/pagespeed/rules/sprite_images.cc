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

#include "pagespeed/rules/sprite_images.h"

#include <string>

#include "base/logging.h"
#include "build/build_config.h"
#include "googleurl/src/gurl.h"
#include "pagespeed/core/formatter.h"
#include "pagespeed/core/image_attributes.h"
#include "pagespeed/core/pagespeed_input.h"
#include "pagespeed/core/resource.h"
#include "pagespeed/core/resource_util.h"
#include "pagespeed/core/result_provider.h"
#include "pagespeed/core/rule_input.h"
#include "pagespeed/core/uri_util.h"
#include "pagespeed/l10n/l10n.h"
#include "pagespeed/proto/pagespeed_output.pb.h"

namespace pagespeed {

namespace rules {

namespace {

const char* kRuleName = "SpriteImages";
const size_t kSpriteImageSizeLimit = 2 * 1024;
const size_t kMinSpriteImageCount = 5;
const size_t kSpriteImagePixelLimit = 96 * 96;

}  // namespace

SpriteImages::SpriteImages()
  : pagespeed::Rule(pagespeed::InputCapabilities(
        pagespeed::InputCapabilities::RESPONSE_BODY |
        pagespeed::InputCapabilities::ONLOAD |
        pagespeed::InputCapabilities::REQUEST_START_TIMES)) {
}

const char* SpriteImages::name() const {
  return kRuleName;
}

UserFacingString SpriteImages::header() const {
  // TRANSLATOR: The name of a Page Speed rule that tells users to try to
  // replace a group of small images on their site with a "CSS sprite" -- that
  // is, a single larger image containing a number of subimages, which can then
  // be used to display a single subimage at a time by using CSS to reveal a
  // different section of the large image in place of each of the original
  // small images.  This is displayed in a list of rule names that Page Speed
  // generates.
  return _("Combine images into CSS sprites");
}

bool SpriteImages::AppendResults(const RuleInput& rule_input,
                                 ResultProvider* provider) {
  typedef std::map<std::string, ResourceSet> DomainResourceMap;
  DomainResourceMap violations;
  const PagespeedInput& input = rule_input.pagespeed_input();
  for (int idx = 0, num = input.num_resources(); idx < num; ++idx) {
    const Resource& resource = input.GetResource(idx);
    const ResourceType resource_type = resource.GetResourceType();
    if (resource_type != IMAGE) {
      continue;
    }

    // Exclude post-onload resources.
    if (input.IsResourceLoadedAfterOnload(resource)) {
      continue;
    }

    // Exclude images other than PNG and GIF.
    const ImageType type = resource.GetImageType();
    if (type != PNG && type != GIF) {
      continue;
    }

    // Exclude big images.
    if (resource.GetResponseBody().size() > kSpriteImageSizeLimit) {
      continue;
    }

    // Exclude images without attributes or 1x1 tracking images.
    scoped_ptr<pagespeed::ImageAttributes> image_attributes(
        input.NewImageAttributes(&resource));
    if (image_attributes == NULL ||
        (image_attributes->GetImageWidth() <= 1 &&
         image_attributes->GetImageHeight() <= 1)) {
      continue;
    }

    // Exclude large images from the set of candidates.
    const size_t num_pixels =
        image_attributes->GetImageWidth() *
        image_attributes->GetImageHeight();
    if (num_pixels > kSpriteImagePixelLimit) {
      continue;
    }

    // Exclude non-cacheable resources.
    if (!resource_util::IsCacheableResource(resource)) {
      continue;
    }

    std::string domain =
        uri_util::GetDomainAndRegistry(resource.GetRequestUrl());
    if (domain.empty()) {
      LOG(INFO) << "Got empty domain for " << resource.GetRequestUrl();
      continue;
    }

    violations[domain].insert(&resource);
  }

  for (DomainResourceMap::const_iterator iter = violations.begin(),
       end = violations.end();
       iter != end;
       ++iter) {
    const ResourceSet& resource_set = iter->second;
    // We allow a small number of independent sprite-able images per domain. For
    // example, the site may have combined many images into 2 sprites. The two
    // images may be able to combine into another one, but there may be other
    // advantages to keep them separate.
    if (resource_set.size() < kMinSpriteImageCount) {
      continue;
    }
    Result* result = provider->NewResult();
    int requests_saved = resource_set.size() - 1;
    for (ResourceSet::const_iterator res_iter = resource_set.begin(),
         res_end = resource_set.end();
         res_iter != res_end;
         ++res_iter) {
      const Resource* resource = *res_iter;
      result->add_resource_urls(resource->GetRequestUrl());
    }
    Savings* savings = result->mutable_savings();
    savings->set_requests_saved(requests_saved);
  }
  return true;
}

void SpriteImages::FormatResults(const ResultVector& results,
                                 RuleFormatter* formatter) {
  UserFacingString body_tmpl =
      // TRANSLATOR: Header at the top of a list of URLs that Page Speed
      // detected as being good candidates to be combined into CSS sprites
      // (that is, larger images containing a number of subimages, which can
      // then be used to display a single subimage at a time by using CSS to
      // reveal a different section of the large image in place of each of the
      // original small images).  It describes the problem to the user, and
      // tells them how to fix it by combining multiple small images into a
      // larger image.  The "$1" is a format string that will be replaced with
      // the URL of the page in which the small images appear.
      _("The following images served from $1 should be combined into as few "
        "images as possible using CSS sprites.");

  if (results.empty()) {
    return;
  }

  for (ResultVector::const_iterator iter = results.begin(),
           end = results.end();
       iter != end;
       ++iter) {
    const Result& result = **iter;
    std::string domain =
        uri_util::GetDomainAndRegistry(result.resource_urls(0));
    UrlBlockFormatter* body = formatter->AddUrlBlock(
        body_tmpl, StringArgument(domain));

    for (int idx = 0; idx < result.resource_urls_size(); idx++) {
      body->AddUrl(result.resource_urls(idx));
    }

  }
}

}  // namespace rules

}  // namespace pagespeed
