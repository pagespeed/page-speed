// Copyright 2011 Google Inc.
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

#ifndef PAGESPEED_DOM_STATISTICS_H_
#define PAGESPEED_DOM_STATISTICS_H_

#include <map>
#include <string>

namespace WebKit {
class WebNode;
class WebDocument;
}

namespace pagespeed {

class DomStatistics {
 public:
  DomStatistics();
  void count(const WebKit::WebDocument& document);
  int TotalNodeCount() const {return total_node_count_;}
  int TotalElementCount() const {return total_element_count_;}
  int MaxDepth() const {return max_depth_;}
  const std::map<std::string, int>& TagCountMap() {
    return tag_count_map_;
  }
 private:
  void countNode(const WebKit::WebNode& node, int depth);
  void countDocument(const WebKit::WebDocument& document);

  int total_node_count_;
  int total_element_count_;
  int max_depth_;
  std::map<std::string, int> tag_count_map_;
};

} // namespace pagespeed
#endif  // PAGESPEED_DOM_STATISTICS_H_
