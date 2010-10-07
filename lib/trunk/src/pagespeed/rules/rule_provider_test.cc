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

#include <vector>

#include "base/stl_util-inl.h"  // for STLElementDeleter
#include "pagespeed/core/rule.h"
#include "pagespeed/rules/rule_provider.h"
#include "testing/gtest/include/gtest/gtest.h"

#include "pagespeed/rules/avoid_bad_requests.h"
#include "pagespeed/rules/avoid_css_import.h"
#include "pagespeed/rules/avoid_document_write.h"
#include "pagespeed/rules/combine_external_resources.h"
#include "pagespeed/rules/enable_gzip_compression.h"
#include "pagespeed/rules/leverage_browser_caching.h"
#include "pagespeed/rules/minify_css.h"
#include "pagespeed/rules/minify_html.h"
#include "pagespeed/rules/minify_javascript.h"
#include "pagespeed/rules/minimize_dns_lookups.h"
#include "pagespeed/rules/minimize_redirects.h"
#include "pagespeed/rules/minimize_request_size.h"
#include "pagespeed/rules/optimize_images.h"
#include "pagespeed/rules/optimize_the_order_of_styles_and_scripts.h"
#include "pagespeed/rules/parallelize_downloads_across_hostnames.h"
#include "pagespeed/rules/prefer_async_resources.h"
#include "pagespeed/rules/put_css_in_the_document_head.h"
#include "pagespeed/rules/remove_query_strings_from_static_resources.h"
#include "pagespeed/rules/serve_resources_from_a_consistent_url.h"
#include "pagespeed/rules/serve_scaled_images.h"
#include "pagespeed/rules/specify_a_cache_validator.h"
#include "pagespeed/rules/specify_a_vary_accept_encoding_header.h"
#include "pagespeed/rules/specify_charset_early.h"
#include "pagespeed/rules/specify_image_dimensions.h"
#include "pagespeed/rules/sprite_images.h"

namespace {

// Call CreateRuleWithName on the given string, then EXPECT both that the call
// succeeds and that the returned object is of the correct type.
void TestNamedRule(const char* name) {
  scoped_ptr<pagespeed::Rule> rule;
  rule.reset(pagespeed::rule_provider::CreateRuleWithName(false, name));
  EXPECT_TRUE(NULL != rule.get());
  EXPECT_STREQ(name, rule->name());
}

}

TEST(RuleProviderTest, CreateRuleWithName) {
  // Test that each rule type is recognized correctly.
  TestNamedRule("AvoidBadRequests");
  TestNamedRule("AvoidCssImport");
  TestNamedRule("AvoidDocumentWrite");
  TestNamedRule("CombineExternalCss");
  TestNamedRule("CombineExternalJavaScript");
  TestNamedRule("EnableGzipCompression");
  TestNamedRule("LeverageBrowserCaching");
  TestNamedRule("MinifyCss");
  TestNamedRule("MinifyHTML");
  TestNamedRule("MinifyJavaScript");
  TestNamedRule("MinimizeDnsLookups");
  TestNamedRule("MinimizeRedirects");
  TestNamedRule("MinimizeRequestSize");
  TestNamedRule("OptimizeImages");
  TestNamedRule("OptimizeTheOrderOfStylesAndScripts");
  TestNamedRule("ParallelizeDownloadsAcrossHostnames");
  TestNamedRule("PreferAsyncResources");
  TestNamedRule("PutCssInTheDocumentHead");
  TestNamedRule("RemoveQueryStringsFromStaticResources");
  TestNamedRule("ServeResourcesFromAConsistentUrl");
  TestNamedRule("ServeScaledImages");
  TestNamedRule("SpecifyACacheValidator");
  TestNamedRule("SpecifyAVaryAcceptEncodingHeader");
  TestNamedRule("SpecifyCharsetEarly");
  TestNamedRule("SpecifyImageDimensions");
  TestNamedRule("SpriteImages");

  // Test that the name-matching is case-insensitive.
  scoped_ptr<pagespeed::Rule> rule;
  rule.reset(pagespeed::rule_provider::CreateRuleWithName(false, "MIniFyCsS"));
  EXPECT_TRUE(NULL != rule.get());
  EXPECT_STREQ("MinifyCss", rule->name());

  // Test that non-existant rules work.
  scoped_ptr<pagespeed::Rule> rule2;
  rule2.reset(pagespeed::rule_provider::CreateRuleWithName(false, "bad_rule"));
  EXPECT_EQ(NULL, rule2.get());
}

TEST(RuleProviderTest, AppendRulesWithNames) {
  // Test successful invokation
  std::vector<pagespeed::Rule*> rules;
  std::vector<std::string> names;
  names.push_back("SpriteImages");
  names.push_back("MinifyHTML");
  names.push_back("AvoidBadRequests");

  STLElementDeleter<std::vector<pagespeed::Rule*> > rule_deleter(&rules);
  EXPECT_TRUE(
      pagespeed::rule_provider::AppendRulesWithNames(false, names, &rules));

  ASSERT_EQ((size_t)3, rules.size());
  EXPECT_STREQ("SpriteImages", rules[0]->name());
  EXPECT_STREQ("MinifyHTML", rules[1]->name());
  EXPECT_STREQ("AvoidBadRequests", rules[2]->name());

  // Test invalid rule names
  std::vector<pagespeed::Rule*> rules2;
  names.clear();
  names.push_back("SpriteImages");
  names.push_back("MinifyHTML");
  names.push_back("bad_rule");
  names.push_back("MinifyCss");

  STLElementDeleter<std::vector<pagespeed::Rule*> > rule_deleter2(&rules2);
  EXPECT_FALSE(
    pagespeed::rule_provider::AppendRulesWithNames(false, names, &rules2));

  ASSERT_EQ((size_t)3, rules2.size());
  EXPECT_STREQ("SpriteImages", rules2[0]->name());
  EXPECT_STREQ("MinifyHTML", rules2[1]->name());
  EXPECT_STREQ("MinifyCss", rules2[2]->name());

  // Test invalid parameters
  EXPECT_FALSE(
      pagespeed::rule_provider::AppendRulesWithNames(false, names, NULL));
}

TEST(RuleProviderTest, AppendAllRules) {
  std::vector<pagespeed::Rule*> rules;
  STLElementDeleter<std::vector<pagespeed::Rule*> > rule_deleter(&rules);
  pagespeed::rule_provider::AppendAllRules(false, &rules);
  ASSERT_FALSE(rules.empty());
}

TEST(RuleProviderTest, AppendCompatibleRulesNone) {
  std::vector<pagespeed::Rule*> rules;
  STLElementDeleter<std::vector<pagespeed::Rule*> > rule_deleter(&rules);
  std::vector<std::string> incompatible_rule_names;
  pagespeed::rule_provider::AppendCompatibleRules(
      false,
      &rules,
      &incompatible_rule_names,
      pagespeed::InputCapabilities());
  // We expect that some rules only require "NONE" while others require more.
  ASSERT_FALSE(rules.empty());
  ASSERT_FALSE(incompatible_rule_names.empty());
}

TEST(RuleProviderTest, AppendCompatibleRulesAll) {
  std::vector<pagespeed::Rule*> rules;
  STLElementDeleter<std::vector<pagespeed::Rule*> > rule_deleter(&rules);
  std::vector<std::string> incompatible_rule_names;
  pagespeed::rule_provider::AppendCompatibleRules(
      false,
      &rules,
      &incompatible_rule_names,
      pagespeed::InputCapabilities(pagespeed::InputCapabilities::ALL));
  ASSERT_TRUE(incompatible_rule_names.empty());

  std::vector<pagespeed::Rule*> all_rules;
  STLElementDeleter<std::vector<pagespeed::Rule*> >
      all_rule_deleter(&all_rules);
  pagespeed::rule_provider::AppendAllRules(false, &all_rules);
  ASSERT_EQ(all_rules.size(), rules.size());
}
