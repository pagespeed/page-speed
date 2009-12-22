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

#include "pagespeed/rules/rule_provider.h"

#include "pagespeed/rules/avoid_bad_requests.h"
#include "pagespeed/rules/combine_external_resources.h"
#include "pagespeed/rules/enable_gzip_compression.h"
#include "pagespeed/rules/minify_javascript.h"
#include "pagespeed/rules/minimize_dns_lookups.h"
#include "pagespeed/rules/minimize_redirects.h"
#include "pagespeed/rules/optimize_images.h"
#include "pagespeed/rules/serve_resources_from_a_consistent_url.h"
#include "pagespeed/rules/specify_image_dimensions.h"

namespace pagespeed {

namespace rule_provider {

void AppendCoreRules(std::vector<Rule*> *rules) {
  rules->push_back(new rules::AvoidBadRequests());
  rules->push_back(new rules::CombineExternalCSS());
  rules->push_back(new rules::CombineExternalJavaScript());
  rules->push_back(new rules::EnableGzipCompression(
      new rules::compression_computer::ZlibComputer()));
  rules->push_back(new rules::MinimizeDnsLookups());
  rules->push_back(new rules::MinimizeRedirects());
  rules->push_back(new rules::ServeResourcesFromAConsistentUrl());
}

void AppendDomRules(std::vector<Rule*> *rules) {
  rules->push_back(new rules::SpecifyImageDimensions);
}

void AppendAllRules(bool save_optimized_content, std::vector<Rule*> *rules) {
  AppendCoreRules(rules);
  AppendDomRules(rules);
  rules->push_back(new rules::MinifyJavaScript(save_optimized_content));
  rules->push_back(new rules::OptimizeImages(save_optimized_content));
}

}  // namespace rule_provider

}  // namespace pagespeed
