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
#include <iterator>

#include "pagespeed/core/string_util.h"
#include "pagespeed/rules/avoid_bad_requests.h"
#include "pagespeed/rules/avoid_charset_in_meta_tag.h"
#include "pagespeed/rules/avoid_css_import.h"
#include "pagespeed/rules/avoid_excess_serialization.h"
#include "pagespeed/rules/avoid_flash_on_mobile.h"
#include "pagespeed/rules/avoid_landing_page_redirects.h"
#include "pagespeed/rules/avoid_long_running_scripts.h"
#include "pagespeed/rules/combine_external_resources.h"
#include "pagespeed/rules/defer_parsing_javascript.h"
#include "pagespeed/rules/eliminate_unnecessary_reflows.h"
#include "pagespeed/rules/enable_gzip_compression.h"
#include "pagespeed/rules/enable_keep_alive.h"
#include "pagespeed/rules/inline_previews_of_visible_images.h"
#include "pagespeed/rules/inline_small_resources.h"
#include "pagespeed/rules/leverage_browser_caching.h"
#include "pagespeed/rules/load_visible_images_first.h"
#include "pagespeed/rules/make_landing_page_redirects_cacheable.h"
#include "pagespeed/rules/minify_css.h"
#include "pagespeed/rules/minify_html.h"
#include "pagespeed/rules/minify_javascript.h"
#include "pagespeed/rules/minimize_dns_lookups.h"
#include "pagespeed/rules/minimize_redirects.h"
#include "pagespeed/rules/minimize_request_size.h"
#include "pagespeed/rules/mobile_viewport.h"
#include "pagespeed/rules/optimize_images.h"
#include "pagespeed/rules/optimize_the_order_of_styles_and_scripts.h"
#include "pagespeed/rules/parallelize_downloads_across_hostnames.h"
#include "pagespeed/rules/prefer_async_resources.h"
#include "pagespeed/rules/put_css_in_the_document_head.h"
#include "pagespeed/rules/remove_query_strings_from_static_resources.h"
#include "pagespeed/rules/serve_resources_from_a_consistent_url.h"
#include "pagespeed/rules/serve_scaled_images.h"
#include "pagespeed/rules/server_response_time.h"
#include "pagespeed/rules/specify_a_cache_validator.h"
#include "pagespeed/rules/specify_a_vary_accept_encoding_header.h"
#include "pagespeed/rules/specify_charset_early.h"
#include "pagespeed/rules/specify_image_dimensions.h"
#include "pagespeed/rules/sprite_images.h"
#include "pagespeed/rules/use_an_application_cache.h"

namespace pagespeed {

namespace rule_provider {

// The names of the Rules in each RuleSet.  Each list must be NULL-terminated
static const char* kCoreRules[] = {
  "avoidbadrequests",
  "avoidcharsetinmetatag",
  "avoidcssimport",
  "avoidexcessserialization",
  "avoidlandingpageredirects",
  "avoidlongrunningscripts",
  "deferparsingjavascript",
  "eliminateunnecessaryreflows",
  "enablegzipcompression",
  "enablekeepalive",
  "inlinesmallcss",
  "inlinesmalljavascript",
  "leveragebrowsercaching",
  "minifycss",
  "minifyhtml",
  "minifyjavascript",
  "minimizeredirects",
  "minimizerequestsize",
  "optimizeimages",
  "optimizetheorderofstylesandscripts",
  "putcssinthedocumenthead",
  "removequerystringsfromstaticresources",
  "serveresourcesfromaconsistenturl",
  "serverresponsetime",
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
  "minimizednslookups",
  "parallelizedownloadsacrosshostnames",
  NULL,
};

static const char* kNewBrowserRules[] = {
  "preferasyncresources",
  NULL,
};

static const char* kMobileBrowserRules[] = {
  // NOTE: Page Speed includes several mobile-targeted rules. However
  // the rules are also applicable to desktop, so they are included as
  // part of the "core" ruleset.
  "avoidflashonmobile",
  "mobileviewport",
  "useanapplicationcache",
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
    case MOBILE_BROWSER_RULES:
      rule_names = kMobileBrowserRules;
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
  if (pagespeed::string_util::LowerCaseEqualsASCII(name, (rule_name))) { \
    return new cnstr; \
  } \
}
  RULE("avoidbadrequests", rules::AvoidBadRequests());
  RULE("avoidcharsetinmetatag", rules::AvoidCharsetInMetaTag());
  RULE("avoidcssimport", rules::AvoidCssImport());
  RULE("avoidexcessserialization", rules::AvoidExcessSerialization());
  RULE("avoidflashonmobile", rules::AvoidFlashOnMobile());
  RULE("avoidlandingpageredirects", rules::AvoidLandingPageRedirects());
  RULE("avoidlongrunningscripts", rules::AvoidLongRunningScripts());
  RULE("combineexternalcss", rules::CombineExternalCss());
  RULE("combineexternaljavascript", rules::CombineExternalJavaScript());
  RULE("deferparsingjavascript", rules::DeferParsingJavaScript());
  RULE("eliminateunnecessaryreflows", rules::EliminateUnnecessaryReflows());
  RULE("enablegzipcompression", rules::EnableGzipCompression());
  RULE("enablekeepalive", rules::EnableKeepAlive());
  RULE("inlinepreviewsofvisibleimages", rules::InlinePreviewsOfVisibleImages());
  RULE("inlinesmallcss", rules::InlineSmallCss());
  RULE("inlinesmalljavascript", rules::InlineSmallJavaScript());
  RULE("leveragebrowsercaching", rules::LeverageBrowserCaching());
  RULE("loadvisibleimagesfirst", rules::LoadVisibleImagesFirst());
  // makelandingpageredirectscacheable was replaced by the
  // avoidlandingpageredirects rule. however we need to continue to
  // make this rule instantiable so old results that contain
  // makelandingpageredirectscacheable entries continue to display
  // properly.
  RULE("makelandingpageredirectscacheable",
       rules::MakeLandingPageRedirectsCacheable());
  RULE("minifycss", rules::MinifyCss(save_optimized_content));
  RULE("minifyhtml", rules::MinifyHTML(save_optimized_content));
  RULE("minifyjavascript", rules::MinifyJavaScript(save_optimized_content));
  RULE("minimizednslookups", rules::MinimizeDnsLookups());
  RULE("minimizeredirects", rules::MinimizeRedirects());
  RULE("minimizerequestsize", rules::MinimizeRequestSize());
  RULE("mobileviewport", rules::MobileViewport());
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
  RULE("serverresponsetime", rules::ServerResponseTime());
  RULE("specifyacachevalidator", rules::SpecifyACacheValidator());
  RULE("specifyavaryacceptencodingheader",
       rules::SpecifyAVaryAcceptEncodingHeader());
  RULE("specifycharsetearly", rules::SpecifyCharsetEarly());
  RULE("specifyimagedimensions", rules::SpecifyImageDimensions());
  RULE("spriteimages", rules::SpriteImages());
  RULE("useanapplicationcache", rules::UseAnApplicationCache());

  // No rule name matched.
  return NULL;
}

bool AppendRulesWithNames(bool save_optimized_content,
                          const std::vector<std::string>& rule_names,
                          std::vector<pagespeed::Rule*>* rules,
                          std::vector<std::string>* nonexistent_rule_names) {
  if (!rules)
    return false;

  bool success = true;
  for (std::vector<std::string>::const_iterator it = rule_names.begin();
       it != rule_names.end();
       ++it) {
    Rule* rule = CreateRuleWithName(save_optimized_content, *it);
    if (rule) {
      rules->push_back(rule);
    } else {
      success = false;
      if (nonexistent_rule_names != NULL) {
        nonexistent_rule_names->push_back(*it);
      }
    }
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
    if (*it && !pagespeed::string_util::StringCaseEqual(name, (*it)->name())) {
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
  for (RuleSet r = kFirstRuleSet; r <= kLastRuleSet;
       r = static_cast<RuleSet>(r + 1)) {
    AppendRuleSet(save_optimized_content, r, rules);
  }
}

void AppendPageSpeedRules(bool save_optimized_content,
                          std::vector<Rule*>* rules) {
  AppendRuleSet(save_optimized_content, CORE_RULES, rules);
  AppendRuleSet(save_optimized_content, NEW_BROWSER_RULES, rules);
}

void AppendCompatibleRules(bool save_optimized_content,
                           std::vector<Rule*>* rules,
                           std::vector<std::string>* incompatible_rule_names,
                           const pagespeed::InputCapabilities& capabilities) {
  AppendAllRules(save_optimized_content, rules);
  RemoveIncompatibleRules(rules, incompatible_rule_names, capabilities);
}

void RemoveIncompatibleRules(std::vector<Rule*>* rules,
                             std::vector<std::string>* incompatible_rule_names,
                             const pagespeed::InputCapabilities& capabilities) {
  std::vector<Rule*> all_rules;
  all_rules.swap(*rules);
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
