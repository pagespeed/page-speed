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
#include "pagespeed/core/resource_util.h"
#include "pagespeed/core/result_provider.h"
#include "pagespeed/core/rule_input.h"
#include "pagespeed/l10n/l10n.h"
#include "pagespeed/proto/pagespeed_output.pb.h"

namespace {

// DOM visitor that finds image nodes and records their coordinates
// in the coordinate space of the top-level document.
class ImageFinder : public pagespeed::ExternalResourceDomElementVisitor {
 public:
  ImageFinder(
      const pagespeed::PagespeedInput* input,
      std::map<const pagespeed::Resource*, std::vector<pagespeed::DomRect> >*
      resource_to_rect_map)
      : input_(input),
        resource_to_rect_map_(resource_to_rect_map),
        x_translate_(0),
        y_translate_(0) {}

  ImageFinder(
      const pagespeed::PagespeedInput* input,
      std::map<const pagespeed::Resource*, std::vector<pagespeed::DomRect> >*
          resource_to_rect_map,
      int x_translate, int y_translate)
      : input_(input),
        resource_to_rect_map_(resource_to_rect_map),
        x_translate_(x_translate),
        y_translate_(y_translate) {}

  virtual void VisitUrl(const pagespeed::DomElement& node, const std::string& url);
  virtual void VisitDocument(const pagespeed::DomElement& node,
                             const pagespeed::DomDocument& document);

 private:
  const pagespeed::PagespeedInput *const input_;
  std::map<const pagespeed::Resource*, std::vector<pagespeed::DomRect> >*
      resource_to_rect_map_;

  // Offset of the current document in the root document's coordinate
  // space.
  int x_translate_;
  int y_translate_;
};

void ImageFinder::VisitUrl(
    const pagespeed::DomElement& node, const std::string& url) {
  const pagespeed::Resource* resource = input_->GetResourceWithUrlOrNull(url);
  if (resource == NULL) {
    LOG(INFO) << "Failed to find resource with URL " << url;
    return;
  }
  if (resource->GetResourceType() == pagespeed::REDIRECT) {
    resource = pagespeed::resource_util::GetLastResourceInRedirectChain(
        *input_, *resource);
    if (resource == NULL) {
      LOG(INFO) << "Failed to traverse redirect chain for URL " << url;
      return;
    }
  }
  if (resource->GetResourceType() != pagespeed::IMAGE) {
    return;
  }
  if (input_->IsResourceLoadedAfterOnload(*resource)) {
    return;
  }

  int x, y, width, height;
  if (node.GetX(&x) == pagespeed::DomElement::SUCCESS &&
      node.GetY(&y) == pagespeed::DomElement::SUCCESS &&
      node.GetActualWidth(&width) == pagespeed::DomElement::SUCCESS &&
      node.GetActualHeight(&height) == pagespeed::DomElement::SUCCESS) {
    (*resource_to_rect_map_)[resource].push_back(
        pagespeed::DomRect(x_translate_ + x, y_translate_ + y, width, height));
  }
}

void ImageFinder::VisitDocument(const pagespeed::DomElement& node,
                                const pagespeed::DomDocument& document) {
  // Get the x and y offsets of the element that hosts the document,
  // so we can translate the document's coordinate space into the root
  // document's coordinate space.
  int x, y;
  if (node.GetX(&x) == pagespeed::DomElement::SUCCESS &&
      node.GetY(&y) == pagespeed::DomElement::SUCCESS) {
    ImageFinder child_finder(input_,
                             resource_to_rect_map_,
                             x_translate_ + x,
                             y_translate_ + y);
    scoped_ptr<pagespeed::DomElementVisitor> child_visitor(
        pagespeed::MakeDomElementVisitorForDocument(&document, &child_finder));
    document.Traverse(child_visitor.get());
  }
}

}  // namespace

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
  const pagespeed::DomRect above_the_fold_rect(
      0, 0, input.viewport_width(), input.viewport_height());

  if (above_the_fold_rect.IsEmpty()) {
    LOG(INFO) << "Received invalid viewport: "
              << input.viewport_width() << "," << input.viewport_height();
    return false;
  }

  std::map<const pagespeed::Resource*, std::vector<pagespeed::DomRect> >
      resource_to_rect_map;
  ImageFinder image_finder(&input, &resource_to_rect_map);
  scoped_ptr<pagespeed::DomElementVisitor> visitor(
      pagespeed::MakeDomElementVisitorForDocument(
          input.dom_document(), &image_finder));
  input.dom_document()->Traverse(visitor.get());

  const pagespeed::Resource* last_requested_above_the_fold_resource = NULL;
  std::vector<const pagespeed::Resource*> not_initially_visible_resources;
  for (std::map<const pagespeed::Resource*,
           std::vector<pagespeed::DomRect> >::const_iterator
           it = resource_to_rect_map.begin(), end = resource_to_rect_map.end();
       it != end; ++it) {
    const pagespeed::Resource& resource = *it->first;
    const std::vector<pagespeed::DomRect>& rects = it->second;

    if (input.IsResourceLoadedAfterOnload(resource)) {
      LOG(DFATAL) << "Only resources loaded before onload "
                  << "should be included in analysis.";
      continue;
    }

    bool above_the_fold = false;
    for(std::vector<pagespeed::DomRect>::const_iterator
            rect_it = rects.begin(), rect_end = rects.end();
        rect_it != rect_end; ++rect_it) {
      const pagespeed::DomRect& candidate = *rect_it;
      if (!candidate.Intersection(above_the_fold_rect).IsEmpty()) {
        above_the_fold = true;
        break;
      }
    }
    if (above_the_fold) {
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
    } else {
      not_initially_visible_resources.push_back(&resource);
    }
  }

  if (last_requested_above_the_fold_resource == NULL) {
    // There are no above the fold image resources, so there is
    // nothing to prioritize relative to.
    return true;
  }

  // Make sure the results are sorted in a consistent order, to
  // guarantee deterministic outputs for tests, etc.
  std::sort(not_initially_visible_resources.begin(),
            not_initially_visible_resources.end());

  for (std::vector<const pagespeed::Resource*>::const_iterator it =
           not_initially_visible_resources.begin(),
           end = not_initially_visible_resources.end(); it != end; ++it) {
    const pagespeed::Resource& candidate = **it;
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
