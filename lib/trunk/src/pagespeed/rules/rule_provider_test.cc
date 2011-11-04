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
#include "pagespeed/rules/combine_external_resources.h"
#include "pagespeed/rules/enable_gzip_compression.h"
#include "pagespeed/rules/enable_keep_alive.h"
#include "pagespeed/rules/inline_small_resources.h"
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

}  // namespace

TEST(RuleProviderTest, AppendRuleSet) {
  using pagespeed::rule_provider::AppendRuleSet;
  using pagespeed::rule_provider::CORE_RULES;
  using pagespeed::rule_provider::OLD_BROWSER_RULES;
  using pagespeed::rule_provider::NEW_BROWSER_RULES;
  using pagespeed::rule_provider::MOBILE_BROWSER_RULES;

  std::vector<pagespeed::Rule*> rules;
  std::vector<pagespeed::Rule*> all_rules;

  STLElementDeleter<std::vector<pagespeed::Rule*> > rule_deleter(&rules);
  STLElementDeleter<std::vector<pagespeed::Rule*> >
      all_rule_deleter(&all_rules);

  // Append each rule set
  EXPECT_TRUE(AppendRuleSet(false, CORE_RULES, &rules));
  EXPECT_TRUE(AppendRuleSet(false, OLD_BROWSER_RULES, &rules));
  EXPECT_TRUE(AppendRuleSet(false, NEW_BROWSER_RULES, &rules));
  EXPECT_TRUE(AppendRuleSet(false, MOBILE_BROWSER_RULES, &rules));

  // Test that each rule is in exactly one RuleSet
  pagespeed::rule_provider::AppendAllRules(false, &all_rules);
  EXPECT_EQ(rules.size(), all_rules.size());

  // Check that each rule in rules occurs in all_rules
  for (std::vector<pagespeed::Rule*>::const_iterator it = rules.begin();
       it != rules.end();
       ++it) {
    const pagespeed::Rule* rule = *it;
    bool found = false;

    for (std::vector<pagespeed::Rule*>::const_iterator it2 = all_rules.begin();
         it2 != all_rules.end();
         ++it2) {
      if (rule->name() == (*it2)->name()) {
        found = true;
        break;
      }
    }

    EXPECT_TRUE(found) << rule->name();
  }
}

TEST(RuleProviderTest, CreateRuleWithName) {
  // Test that each rule type is recognized correctly.
  TestNamedRule("AvoidBadRequests");
  TestNamedRule("AvoidCssImport");
  TestNamedRule("CombineExternalCss");
  TestNamedRule("CombineExternalJavaScript");
  TestNamedRule("EnableGzipCompression");
  TestNamedRule("EnableKeepAlive");
  TestNamedRule("InlineSmallCss");
  TestNamedRule("InlineSmallJavaScript");
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
  std::vector<pagespeed::Rule*> rules;
  std::vector<std::string> names;
  names.push_back("SpriteImages");
  names.push_back("MinifyHTML");
  names.push_back("AvoidBadRequests");

  STLElementDeleter<std::vector<pagespeed::Rule*> > rule_deleter(&rules);
  EXPECT_TRUE(
      pagespeed::rule_provider::AppendRulesWithNames(
          false, names, &rules, NULL));

  ASSERT_EQ((size_t)3, rules.size());
  EXPECT_STREQ("SpriteImages", rules[0]->name());
  EXPECT_STREQ("MinifyHTML", rules[1]->name());
  EXPECT_STREQ("AvoidBadRequests", rules[2]->name());
}

TEST(RuleProviderTest, AppendRulesWithNamesInvalidRule) {
  std::vector<pagespeed::Rule*> rules;
  std::vector<std::string> names;
  names.push_back("SpriteImages");
  names.push_back("MinifyHTML");
  names.push_back("bad_rule");
  names.push_back("MinifyCss");

  STLElementDeleter<std::vector<pagespeed::Rule*> > rule_deleter(&rules);
  EXPECT_FALSE(
      pagespeed::rule_provider::AppendRulesWithNames(
          false, names, &rules, NULL));

  ASSERT_EQ((size_t)3, rules.size());
  EXPECT_STREQ("SpriteImages", rules[0]->name());
  EXPECT_STREQ("MinifyHTML", rules[1]->name());
  EXPECT_STREQ("MinifyCss", rules[2]->name());
}

TEST(RuleProviderTest, AppendRulesWithNamesNonexistentRuleNames) {
  std::vector<pagespeed::Rule*> rules;
  std::vector<std::string> names;
  names.push_back("MinifyHTML");
  names.push_back("bad_rule");
  names.push_back("MinifyCss");

  STLElementDeleter<std::vector<pagespeed::Rule*> > rule_deleter(&rules);
  std::vector<std::string> nonexistent_rule_names;
  EXPECT_FALSE(
      pagespeed::rule_provider::AppendRulesWithNames(
          false, names, &rules, &nonexistent_rule_names));

  ASSERT_EQ((size_t)2, rules.size());
  EXPECT_STREQ("MinifyHTML", rules[0]->name());
  EXPECT_STREQ("MinifyCss", rules[1]->name());
  ASSERT_EQ((size_t)1, nonexistent_rule_names.size());
  EXPECT_STREQ("bad_rule", nonexistent_rule_names[0].c_str());
}

TEST(RuleProviderTest, AppendRulesWithNamesNonexistentRuleNamesNotEmpty) {
  std::vector<pagespeed::Rule*> rules;
  std::vector<std::string> names;
  names.push_back("MinifyHTML");
  names.push_back("bad_rule");
  names.push_back("MinifyCss");

  STLElementDeleter<std::vector<pagespeed::Rule*> > rule_deleter(&rules);

  // Intentionally add a value to nonexistent_rule_names before
  // invoking, to verify correct error handling behavior.
  std::vector<std::string> nonexistent_rule_names;
  nonexistent_rule_names.push_back("not_empty");
  EXPECT_FALSE(
      pagespeed::rule_provider::AppendRulesWithNames(
          false, names, &rules, &nonexistent_rule_names));
  ASSERT_EQ((size_t)2, rules.size());
  EXPECT_STREQ("MinifyHTML", rules[0]->name());
  EXPECT_STREQ("MinifyCss", rules[1]->name());
  ASSERT_EQ((size_t)2, nonexistent_rule_names.size());
  EXPECT_STREQ("not_empty", nonexistent_rule_names[0].c_str());
  EXPECT_STREQ("bad_rule", nonexistent_rule_names[1].c_str());
}

TEST(RuleProviderTest, AppendRulesWithNamesBadParams) {
  std::vector<std::string> names;
  names.push_back("MinifyHTML");
  EXPECT_FALSE(
      pagespeed::rule_provider::AppendRulesWithNames(
          false, names, NULL, NULL));
}

TEST(RuleProviderTest, RemoveRuleWithName) {
  using pagespeed::rule_provider::CreateRuleWithName;
  using pagespeed::rule_provider::RemoveRuleWithName;

  std::vector<pagespeed::Rule*> rules;
  rules.push_back(CreateRuleWithName(false, "SpriteImages"));
  rules.push_back(CreateRuleWithName(false, "MinifyHTML"));
  rules.push_back(CreateRuleWithName(false, "AvoidBadRequests"));
  // Add the same rule twice to test for leaks
  rules.push_back(CreateRuleWithName(false, "MinifyHTML"));

  STLElementDeleter<std::vector<pagespeed::Rule*> > rule_deleter(&rules);

  pagespeed::Rule* removed_rule = NULL;
  RemoveRuleWithName("MinifyHTML", &rules, &removed_rule);
  ASSERT_TRUE(removed_rule != NULL);
  delete removed_rule;
  removed_rule = NULL;

  EXPECT_EQ((size_t)3, rules.size());
  EXPECT_STREQ("SpriteImages", rules[0]->name());
  EXPECT_STREQ("AvoidBadRequests", rules[1]->name());
  EXPECT_STREQ("MinifyHTML", rules[2]->name());

  // Test invalid rule name
  EXPECT_FALSE(RemoveRuleWithName("bad_rule", &rules, &removed_rule));
  EXPECT_EQ(NULL, removed_rule);

  EXPECT_EQ((size_t)3, rules.size());
  EXPECT_STREQ("SpriteImages", rules[0]->name());
  EXPECT_STREQ("AvoidBadRequests", rules[1]->name());
  EXPECT_STREQ("MinifyHTML", rules[2]->name());
}

TEST(RuleProviderTest, AppendAllRules) {
  std::vector<pagespeed::Rule*> rules;
  STLElementDeleter<std::vector<pagespeed::Rule*> > rule_deleter(&rules);
  pagespeed::rule_provider::AppendAllRules(false, &rules);
  ASSERT_FALSE(rules.empty());
}

TEST(RuleProviderTest, AllInputCapabilities) {
  std::vector<pagespeed::Rule*> rules;
  pagespeed::rule_provider::AppendAllRules(false, &rules);
  std::vector<pagespeed::Rule*> compatible_rules(rules);
  STLElementDeleter<std::vector<pagespeed::Rule*> > rule_deleter(
      &compatible_rules);

  std::vector<std::string> incompatible_rule_names;
  pagespeed::rule_provider::RemoveIncompatibleRules(
      &compatible_rules,
      &incompatible_rule_names,
      pagespeed::InputCapabilities(pagespeed::InputCapabilities::ALL));
  ASSERT_TRUE(incompatible_rule_names.empty());
  ASSERT_EQ(rules.size(), compatible_rules.size());
}

TEST(RuleProviderTest, NoInputCapabilities) {
  std::vector<pagespeed::Rule*> rules;
  pagespeed::rule_provider::AppendAllRules(false, &rules);
  std::vector<pagespeed::Rule*> compatible_rules(rules);
  STLElementDeleter<std::vector<pagespeed::Rule*> > rule_deleter(
      &compatible_rules);

  std::vector<std::string> incompatible_rule_names;
  pagespeed::rule_provider::RemoveIncompatibleRules(
      &compatible_rules,
      &incompatible_rule_names,
      pagespeed::InputCapabilities());
  // We expect that some rules require no capabilities, while others
  // require some capabilities. Thus the resulting vector should
  // include some but not all of the original rules.
  ASSERT_FALSE(compatible_rules.empty());
  ASSERT_FALSE(incompatible_rule_names.empty());
  ASSERT_GT(rules.size(), compatible_rules.size());
}
