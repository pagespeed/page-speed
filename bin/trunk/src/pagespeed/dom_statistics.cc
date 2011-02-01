// Copyright 2010 Google Inc.
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


#include "pagespeed/dom_statistics.h"

#include "third_party/WebKit/WebKit/chromium/public/WebDocument.h"
#include "third_party/WebKit/WebKit/chromium/public/WebElement.h"
#include "third_party/WebKit/WebKit/chromium/public/WebFrame.h"
#include "third_party/WebKit/WebKit/chromium/public/WebString.h"

namespace pagespeed {

namespace {

}  // namespace


DomStatistics::DomStatistics()
  : total_node_count_(0),
    total_element_count_(0),
    max_depth_(0) {
}

void DomStatistics::countNode(const WebKit::WebNode& node, int depth) {
  if (node.isNull()) {
    return;
  }
  ++total_node_count_;

  if (node.isElementNode()) {
    WebKit::WebElement element = node.toConst<WebKit::WebElement>();
    std::string tag_name = element.tagName().utf8();
    ++tag_count_map_[tag_name];
    ++total_element_count_;
  }

  if (depth > max_depth_) {
    max_depth_ = depth;
  }

  WebKit::WebNode child = node.firstChild();
  while (!child.isNull()) {
    countNode(child, depth+1);
    child = child.nextSibling();
  }
}

void DomStatistics::countDocument(const WebKit::WebDocument& document) {
   const WebKit::WebNode root = document.documentElement();
   max_depth_ = 0;
   countNode(root, 1);
}

void DomStatistics::count(const WebKit::WebDocument& document) {
  countDocument(document);
}

}  // namespace pagespeed
