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

#include "pagespeed/dom/resource_coordinate_finder.h"

#include <algorithm>

#include "base/logging.h"
#include "pagespeed/core/dom.h"
#include "pagespeed/core/pagespeed_input.h"
#include "pagespeed/core/resource.h"
#include "pagespeed/core/resource_util.h"

namespace pagespeed {
namespace dom {

void ResourceCoordinateFinder::VisitUrl(
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
  // For now, we only try to get coordinates for images. We may expand
  // this list to include other resource types in the future,
  // e.g. flash.
  if (resource->GetResourceType() != pagespeed::IMAGE) {
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

void ResourceCoordinateFinder::VisitDocument(
    const pagespeed::DomElement& node,
    const pagespeed::DomDocument& document) {
  // Get the x and y offsets of the element that hosts the document,
  // so we can translate the document's coordinate space into the root
  // document's coordinate space.
  int x, y;
  if (node.GetX(&x) == pagespeed::DomElement::SUCCESS &&
      node.GetY(&y) == pagespeed::DomElement::SUCCESS) {
    ResourceCoordinateFinder child_finder(input_,
                             resource_to_rect_map_,
                             x_translate_ + x,
                             y_translate_ + y);
    scoped_ptr<pagespeed::DomElementVisitor> child_visitor(
        pagespeed::MakeDomElementVisitorForDocument(&document, &child_finder));
    document.Traverse(child_visitor.get());
  }
}

bool FindOnAndOffscreenImageResources(
    const PagespeedInput& input,
    std::vector<const pagespeed::Resource*>* out_onscreen_resources,
    std::vector<const pagespeed::Resource*>* out_offscreen_resources) {
  const pagespeed::DomRect onscreen_rect(0,
                                         0,
                                         input.viewport_width(),
                                         input.viewport_height());
  if (onscreen_rect.IsEmpty()) {
    LOG(INFO) << "Received empty onscreen rect: "
              << onscreen_rect.width() << "," << onscreen_rect.height();
    return false;
  }

  if (!out_onscreen_resources->empty()) {
    LOG(DFATAL) << "out_onscreen_resources non-empty.";
    out_onscreen_resources->clear();
  }

  if (!out_offscreen_resources->empty()) {
    LOG(DFATAL) << "out_offscreen_resources non-empty.";
    out_offscreen_resources->clear();
  }

  std::map<const pagespeed::Resource*, std::vector<pagespeed::DomRect> >
      resource_to_rect_map;
  ResourceCoordinateFinder image_finder(&input, &resource_to_rect_map);
  scoped_ptr<pagespeed::DomElementVisitor> visitor(
      pagespeed::MakeDomElementVisitorForDocument(
          input.dom_document(), &image_finder));
  input.dom_document()->Traverse(visitor.get());

  for (std::map<const pagespeed::Resource*,
           std::vector<pagespeed::DomRect> >::const_iterator
           it = resource_to_rect_map.begin(), end = resource_to_rect_map.end();
       it != end; ++it) {
    const pagespeed::Resource& resource = *it->first;
    const std::vector<pagespeed::DomRect>& rects = it->second;

    bool is_onscreen = false;
    for(std::vector<pagespeed::DomRect>::const_iterator
            rect_it = rects.begin(), rect_end = rects.end();
        rect_it != rect_end; ++rect_it) {
      const pagespeed::DomRect& candidate = *rect_it;
      if (!candidate.Intersection(onscreen_rect).IsEmpty()) {
        is_onscreen = true;
        break;
      }
    }
    if (is_onscreen) {
      out_onscreen_resources->push_back(&resource);
    } else {
      out_offscreen_resources->push_back(&resource);
    }
  }

  // Make sure the results are sorted in a consistent order, to
  // guarantee deterministic output.
  std::sort(out_onscreen_resources->begin(),
            out_onscreen_resources->end(),
            pagespeed::ResourceUrlLessThan());
  std::sort(out_offscreen_resources->begin(),
            out_offscreen_resources->end(),
            pagespeed::ResourceUrlLessThan());

  return true;
}

}  // namespace dom
}  // namespace pagespeed

