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

#include <algorithm>

#include "base/string_util.h"
#include "pagespeed/rules/avoid_bad_requests.h"
#include "pagespeed/rules/avoid_css_import.h"
#include "pagespeed/rules/avoid_document_write.h"
#include "pagespeed/rules/combine_external_resources.h"
#include "pagespeed/rules/enable_gzip_compression.h"
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

namespace pagespeed {

namespace rule_provider {

// The names of the Rules in each RuleSet.  Each list must be NULL-terminated
static const char* kCoreRules[] = {
  "avoidbadrequests",
  "avoidcssimport",
  "avoiddocumentwrite",
  "enablegzipcompression",
  "inlinesmallcss",
  "inlinesmalljavascript",
  "leveragebrowsercaching",
  "minifycss",
  "minifyhtml",
  "minifyjavascript",
  "minimizednslookups",
  "minimizeredirects",
  "minimizerequestsize",
  "optimizeimages",
  "optimizetheorderofstylesandscripts",
  "putcssinthedocumenthead",
  "removequerystringsfromstaticresources",
  "serveresourcesfromaconsistenturl",
  "servescaledimages",
  "specifyacachevalidator",
  "specifyavaryacceptencodingheader",
  "specifycharsetearly",
  "specifyimagedimensions",
  "spriteimages",
  NULL,
};

static const char* kOldBrowserRules[] = {
  "combineexternalcss",
  "combineexternaljavascript",
  "parallelizedownloadsacrosshostnames",
  NULL,
};

static const char* kNewBrowserRules[] = {
  "preferasyncresources",
  NULL,
};

static const char* kExperimentalRules[] = {
  NULL,
};

bool AppendRuleSet(bool save_optimized_content, RuleSet ruleset,
                   std::vector<Rule*>* rules) {
  const char** rule_names = NULL;
  bool success = true;

  if (!rules)
    return false;

  switch (ruleset) {
    case CORE_RULES:
      rule_names = kCoreRules;
      break;
    case OLD_BROWSER_RULES:
      rule_names = kOldBrowserRules;
      break;
    case NEW_BROWSER_RULES:
      rule_names = kNewBrowserRules;
      break;
    case EXPERIMENTAL_RULES:
      rule_names = kExperimentalRules;
      break;
    default:
      return false;
  }

  while (*rule_names != NULL) {
    const char* name = *rule_names;
    Rule* rule = CreateRuleWithName(save_optimized_content, name);
    if (!rule)
      success = false;
    else
      rules->push_back(rule);
    rule_names++;
  }

  return success;
}

// Note: keep this methed (and tests) up-to-date with the active set of rules.
Rule* CreateRuleWithName(bool save_optimized_content, const std::string& name) {
  // Compare the function parameter name to the given rule name, and construct a
  // new rule of the appropriate type if they match
#define RULE(rule_name, cnstr) { \
  if (LowerCaseEqualsASCII(name, (rule_name))) { \
    return new cnstr; \
  } \
}
  RULE("avoidbadrequests", rules::AvoidBadRequests());
  RULE("avoidcssimport", rules::AvoidCssImport());
  RULE("avoiddocumentwrite", rules::AvoidDocumentWrite());
  RULE("combineexternalcss", rules::CombineExternalCss());
  RULE("combineexternaljavascript", rules::CombineExternalJavaScript());
  RULE("enablegzipcompression", rules::EnableGzipCompression(
      new rules::compression_computer::ZlibComputer()));
  RULE("inlinesmallcss", rules::InlineSmallCss());
  RULE("inlinesmalljavascript", rules::InlineSmallJavaScript());
  RULE("leveragebrowsercaching", rules::LeverageBrowserCaching());
  RULE("minifycss", rules::MinifyCss(save_optimized_content));
  RULE("minifyhtml", rules::MinifyHTML(save_optimized_content));
  RULE("minifyjavascript", rules::MinifyJavaScript(save_optimized_content));
  RULE("minimizednslookups", rules::MinimizeDnsLookups());
  RULE("minimizeredirects", rules::MinimizeRedirects());
  RULE("minimizerequestsize", rules::MinimizeRequestSize());
  RULE("optimizeimages", rules::OptimizeImages(save_optimized_content));
  RULE("optimizetheorderofstylesandscripts",
       rules::OptimizeTheOrderOfStylesAndScripts());
  RULE("parallelizedownloadsacrosshostnames",
       rules::ParallelizeDownloadsAcrossHostnames());
  RULE("preferasyncresources", rules::PreferAsyncResources());
  RULE("putcssinthedocumenthead", rules::PutCssInTheDocumentHead());
  RULE("removequerystringsfromstaticresources",
       rules::RemoveQueryStringsFromStaticResources());
  RULE("serveresourcesfromaconsistenturl",
       rules::ServeResourcesFromAConsistentUrl());
  RULE("servescaledimages", rules::ServeScaledImages());
  RULE("specifyacachevalidator", rules::SpecifyACacheValidator());
  RULE("specifyavaryacceptencodingheader",
       rules::SpecifyAVaryAcceptEncodingHeader());
  RULE("specifycharsetearly", rules::SpecifyCharsetEarly());
  RULE("specifyimagedimensions", rules::SpecifyImageDimensions());
  RULE("spriteimages", rules::SpriteImages());

  // No rule name matched.
  return NULL;
}

bool AppendRulesWithNames(bool save_optimized_content,
                          const std::vector<std::string>& rule_names,
                          std::vector<pagespeed::Rule*>* rules) {
  if (!rules)
    return false;

  bool success = true;
  for (std::vector<std::string>::const_iterator it = rule_names.begin();
       it != rule_names.end();
       ++it) {
    Rule* rule = CreateRuleWithName(save_optimized_content, *it);
    if (rule)
      rules->push_back(rule);
    else
      success = false;
  }
  return success;
}

bool RemoveRuleWithName(const std::string& name, std::vector<Rule*>* rules,
                        Rule** removed_rule) {
  if (!rules || !removed_rule)
    return false;

  // Construct a new vector<Rule*> on the stack, then copy it over when finished
  bool success = false;
  std::vector<Rule*> out_rules;
  for (std::vector<Rule*>::iterator it = rules->begin();
       it != rules->end();
       ++it) {
    if (*it && base::strcasecmp(name.c_str(), (*it)->name()) != 0) {
      out_rules.push_back(*it);
    } else {
      *removed_rule = *it;
      success = true;
      // Copy over the rest of the rules
      ++it;
      std::copy(it, rules->end(),
                std::back_insert_iterator< std::vector<Rule*> >(out_rules));
      break;
    }
  }

  *rules = out_rules;
  return success;
}

void AppendAllRules(bool save_optimized_content, std::vector<Rule*>* rules) {
  rules->push_back(new rules::AvoidBadRequests());
  rules->push_back(new rules::AvoidCssImport());
  rules->push_back(new rules::AvoidDocumentWrite());
  rules->push_back(new rules::CombineExternalCss());
  rules->push_back(new rules::CombineExternalJavaScript());
  rules->push_back(new rules::EnableGzipCompression(
      new rules::compression_computer::ZlibComputer()));
  rules->push_back(new rules::InlineSmallCss());
  rules->push_back(new rules::InlineSmallJavaScript());
  rules->push_back(new rules::LeverageBrowserCaching());
  rules->push_back(new rules::MinifyCss(save_optimized_content));
  rules->push_back(new rules::MinifyHTML(save_optimized_content));
  rules->push_back(new rules::MinifyJavaScript(save_optimized_content));
  rules->push_back(new rules::MinimizeDnsLookups());
  rules->push_back(new rules::MinimizeRedirects());
  rules->push_back(new rules::MinimizeRequestSize());
  rules->push_back(new rules::OptimizeImages(save_optimized_content));
  rules->push_back(new rules::OptimizeTheOrderOfStylesAndScripts());
  rules->push_back(new rules::ParallelizeDownloadsAcrossHostnames());
  rules->push_back(new rules::PreferAsyncResources());
  rules->push_back(new rules::PutCssInTheDocumentHead());
  rules->push_back(new rules::RemoveQueryStringsFromStaticResources());
  rules->push_back(new rules::ServeResourcesFromAConsistentUrl());
  rules->push_back(new rules::ServeScaledImages());
  rules->push_back(new rules::SpecifyACacheValidator());
  rules->push_back(new rules::SpecifyAVaryAcceptEncodingHeader());
  rules->push_back(new rules::SpecifyCharsetEarly());
  rules->push_back(new rules::SpecifyImageDimensions());
  rules->push_back(new rules::SpriteImages());
}

void AppendCompatibleRules(bool save_optimized_content,
                           std::vector<Rule*>* rules,
                           std::vector<std::string>* incompatible_rule_names,
                           const pagespeed::InputCapabilities& capabilities) {
  std::vector<Rule*> all_rules;
  AppendAllRules(save_optimized_content, &all_rules);
  for (std::vector<Rule*>::const_iterator it = all_rules.begin(),
           end = all_rules.end();
       it != end;
       ++it) {
    Rule* r = *it;
    if (capabilities.satisfies(r->capability_requirements())) {
      rules->push_back(r);
    } else {
      incompatible_rule_names->push_back(r->name());
      delete r;
    }
  }
}

}  // namespace rule_provider

}  // namespace pagespeed
