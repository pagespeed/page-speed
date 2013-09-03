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

#include "pagespeed_chromium/pagespeed_chromium.h"

#include <string>
#include <vector>

#include "base/base64.h"
#include "base/basictypes.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/logging.h"
#include "base/md5.h"
#include "base/memory/scoped_ptr.h"
#include "base/stl_util.h"
#include "base/string_number_conversions.h"
#include "base/string_util.h"
#include "base/values.h"
#include "googleurl/src/gurl.h"
#include "pagespeed/core/engine.h"
#include "pagespeed/core/file_util.h"
#include "pagespeed/core/formatter.h"
#include "pagespeed/core/pagespeed_init.h"
#include "pagespeed/core/pagespeed_input.h"
#include "pagespeed/core/pagespeed_input_util.h"
#include "pagespeed/core/resource_filter.h"
#include "pagespeed/core/rule.h"
#include "pagespeed/dom/json_dom.h"
#include "pagespeed/timeline/json_importer.h"
#include "pagespeed/filters/ad_filter.h"
#include "pagespeed/filters/response_byte_result_filter.h"
#include "pagespeed/filters/landing_page_redirection_filter.h"
#include "pagespeed/filters/tracker_filter.h"
#include "pagespeed/formatters/proto_formatter.h"
#include "pagespeed/har/http_archive.h"
#include "pagespeed/image_compression/image_attributes_factory.h"
#include "pagespeed/l10n/gettext_localizer.h"
#include "pagespeed/l10n/localizer.h"
#include "pagespeed/proto/formatted_results_to_json_converter.h"
#include "pagespeed/proto/pagespeed_output.pb.h"
#include "pagespeed/proto/pagespeed_proto_formatter.pb.h"
#include "pagespeed/proto/timeline.pb.h"
#include "pagespeed/rules/rule_provider.h"

namespace {

pagespeed::ResourceFilter* NewFilter(const std::string& analyze) {
  if (analyze == "ads") {
    return new pagespeed::NotResourceFilter(new pagespeed::AdFilter());
  } else if (analyze == "trackers") {
    return new pagespeed::NotResourceFilter(new pagespeed::TrackerFilter());
  } else if (analyze == "content") {
    return new pagespeed::AndResourceFilter(new pagespeed::AdFilter(),
                                            new pagespeed::TrackerFilter());
  } else {
    if (analyze != "all") {
      LOG(DFATAL) << "Unknown filter type: " << analyze;
    }
    return new pagespeed::AllowAllResourceFilter();
  }
}

void SerializeOptimizedContent(const pagespeed::Results& results,
                               base::DictionaryValue* optimized_content) {
  for (int i = 0; i < results.rule_results_size(); ++i) {
    const pagespeed::RuleResults& rule_results = results.rule_results(i);
    for (int j = 0; j < rule_results.results_size(); ++j) {
      const pagespeed::Result& result = rule_results.results(j);
      if (!result.has_optimized_content()) {
        continue;
      }

      const std::string key = base::IntToString(result.id());
      if (optimized_content->HasKey(key)) {
        LOG(ERROR) << "Duplicate result id: " << key;
        continue;
      }

      if (result.resource_urls_size() <= 0) {
        LOG(ERROR) << "Result id " << key
                   << " has optimized content, but no resource URLs";
        continue;
      }

      const std::string& url = result.resource_urls(0);
      const GURL gurl(url);
      if (!gurl.is_valid()) {
        LOG(ERROR) << "Invalid url: " << url;
        continue;
      }

      // TODO(mdsteele): Maybe we shouldn't base64-encode HTML/JS/CSS files?
      const std::string& content = result.optimized_content();
      std::string encoded;
      if (!base::Base64Encode(content, &encoded)) {
        LOG(ERROR) << "Base64Encode failed for " << url;
        continue;
      }

      const std::string& mimetype = result.optimized_content_mime_type();
      scoped_ptr<base::DictionaryValue> entry(new base::DictionaryValue());
      entry->SetString("filename", pagespeed::ChooseOutputFilename(
          gurl, mimetype, base::MD5String(content)));
      entry->SetString("mimetype", mimetype);
      entry->SetString("content", encoded);
      optimized_content->Set(key, entry.release());
    }
  }
}

}  // namespace

namespace pagespeed_chromium {

bool RunPageSpeedRules(const std::string& data,
                       std::string* output_string,
                       std::string* error_string) {
  // NOTE: this could be made more efficient by representing the
  // sub-values as JSON objects rather than strings, and by having a
  // common RunPageSpeedRules method that takes Value objects. This is
  // complicated by the fact that some of the values (i.e. the
  // document value) have ownership transferred to their JSON parser
  // (e.g. pagespeed::dom::CreateDocument). For now, we take the less
  // efficient but simpler approach of encoding the sub-values as
  // strings.
  scoped_ptr<const base::Value> data_json(base::JSONReader::ReadAndReturnError(
      data,
      true,  // allow_trailing_comma
      NULL,  // error_code_out (ReadAndReturnError permits NULL here)
      error_string));
  if (data_json == NULL) {
    return false;
  }

  if (!data_json->IsType(base::Value::TYPE_DICTIONARY)) {
    *error_string = "Input is not a JSON dictionary.";
    return false;
  }

  const base::DictionaryValue* root =
      static_cast<const base::DictionaryValue*>(data_json.get());
  std::string id, har_data, document_data, timeline_data,
      resource_filter_name, locale;
  bool save_optimized_content;
  if (!root->GetString("id", &id) ||
      !root->GetString("har", &har_data) ||
      !root->GetString("document", &document_data) ||
      !root->GetString("timeline", &timeline_data) ||
      !root->GetString("resource_filter", &resource_filter_name) ||
      !root->GetString("locale", &locale) ||
      !root->GetBoolean("save_optimized_content", &save_optimized_content)) {
    *error_string = "Failed to extract required field(s) from input JSON.";
    return false;
  }

  bool is_mobile = false;
  if (!root->GetBoolean("mobile", &is_mobile)) {
    LOG(INFO) << "Input JSON does not have MOBILE info.";
  }
  return RunPageSpeedRules(id,
                           har_data,
                           document_data,
                           timeline_data,
                           resource_filter_name,
                           locale,
                           is_mobile,
                           save_optimized_content,
                           output_string,
                           error_string);
}


// Parse the HAR data and run the Page Speed rules, then format the results.
// Return false if the HAR data could not be parsed, true otherwise.
// This function will take ownership of the filter and document arguments, and
// will delete them before returning.
bool RunPageSpeedRules(const std::string& id,
                       const std::string& har_data,
                       const std::string& document_data,
                       const std::string& timeline_data,
                       const std::string& resource_filter_name,
                       const std::string& locale,
                       bool is_mobile,
                       bool save_optimized_content,
                       std::string* output_string,
                       std::string* error_string) {
  // Parse the HAR into a PagespeedInput object.  ParseHttpArchiveWithFilter
  // will ensure that filter gets deleted.
  scoped_ptr<pagespeed::PagespeedInput> input(
      pagespeed::ParseHttpArchiveWithFilter(
          har_data, NewFilter(resource_filter_name)));
  if (input.get() == NULL) {
    *error_string = "could not parse HAR";
    return false;
  }

  pagespeed::InstrumentationDataVector timeline_protos;
  STLElementDeleter<pagespeed::InstrumentationDataVector>
      timeline_deleter(&timeline_protos);
  if (!pagespeed::timeline::CreateTimelineProtoFromJsonString(
          timeline_data, &timeline_protos)) {
    *error_string = "error in timeline data";
    return false;
  }

  std::string error_msg_out;
  scoped_ptr<const base::Value> document_json(
      base::JSONReader::ReadAndReturnError(
      document_data,
      true,  // allow_trailing_comma
      NULL,  // error_code_out (ReadAndReturnError permits NULL here)
      &error_msg_out));
  if (document_json == NULL) {
    *error_string = "could not parse DOM: " + error_msg_out;
    return false;
  }
  if (!document_json->IsType(base::Value::TYPE_DICTIONARY)) {
    *error_string = "DOM must be a JSON dictionary";
    return false;
  }

  // Ownership of the document_json is transferred to the returned
  // DomDocument instance.
  pagespeed::DomDocument* document = pagespeed::dom::CreateDocument(
      static_cast<const base::DictionaryValue*>(document_json.release()));

  // Add the DOM document to the PagespeedInput object.
  if (document != NULL) {
    input->SetPrimaryResourceUrl(document->GetDocumentUrl());
  }
  input->AcquireDomDocument(document);  // input takes ownership of document

  // Finish up the PagespeedInput object and freeze it.
  input->AcquireInstrumentationData(&timeline_protos);
  input->AcquireImageAttributesFactory(
      new pagespeed::image_compression::ImageAttributesFactory());

  std::vector<pagespeed::Rule*> rules;

  // In environments where exceptions can be thrown, use
  // STLElementDeleter to make sure we free the rules in the event
  // that they are not transferred to the Engine.
  STLElementDeleter<std::vector<pagespeed::Rule*> > rule_deleter(&rules);

  pagespeed::rule_provider::AppendPageSpeedRules(
      save_optimized_content, &rules);
  if (is_mobile) {
    pagespeed::ClientCharacteristics client_characteristics;
    pagespeed::pagespeed_input_util::PopulateMobileClientCharacteristics(
        &client_characteristics);
    input->SetClientCharacteristics(client_characteristics);
    pagespeed::rule_provider::AppendRuleSet(save_optimized_content,
        pagespeed::rule_provider::MOBILE_BROWSER_RULES, &rules);
  }
  input->Freeze();

  std::vector<std::string> incompatible_rule_names;
  pagespeed::rule_provider::RemoveIncompatibleRules(
      &rules, &incompatible_rule_names, input->EstimateCapabilities());
  if (!incompatible_rule_names.empty()) {
    LOG(INFO) << "Removing incompatible rules: "
              << JoinString(incompatible_rule_names, ' ');
  }

  // Ownership of rules is transferred to the Engine instance.
  pagespeed::Engine engine(&rules);
  engine.Init();

  // Compute results.
  pagespeed::Results unfiltered_results;
  if (!engine.ComputeResults(*input, &unfiltered_results)) {
    std::vector<std::string> error_rules;
    for (int i = 0, size = unfiltered_results.error_rules_size(); i < size;
         ++i) {
      error_rules.push_back(unfiltered_results.error_rules(i));
    }
    LOG(WARNING) << "Errors during ComputeResults in rules: "
                 << JoinString(error_rules, ' ');
  }

  // Filter out the results of some landing page redirection rules. For example,
  // user typed url foo.com -> www.foo.com redirection is allowed.
  pagespeed::LandingPageRedirectionFilter redirection_filter;
  pagespeed::Results filtered_results;
  engine.FilterResults(unfiltered_results, redirection_filter,
                       &filtered_results);

  // Format results.
  pagespeed::FormattedResults formatted_results;
  {
    scoped_ptr<pagespeed::l10n::Localizer> localizer(
        pagespeed::l10n::GettextLocalizer::Create(locale));
    if (localizer.get() == NULL) {
      LOG(WARNING) << "Could not create GettextLocalizer for " << locale;
      localizer.reset(new pagespeed::l10n::BasicLocalizer);
    }

    formatted_results.set_locale(localizer->GetLocale());
    pagespeed::formatters::ProtoFormatter formatter(localizer.get(),
                                                    &formatted_results);
    pagespeed::ResponseByteResultFilter result_filter;
    if (!engine.FormatResults(filtered_results, result_filter, &formatter)) {
      *error_string = "error during FormatResults";
      return false;
    }
  }

  // The ResponseByteResultFilter may filter some results. In the
  // event that all results are filtered from a FormattedRuleResults,
  // we update its score to 100 and impact to 0, to reflect the fact
  // that we are not showing any suggestions. Likewise, if we find no
  // results in any rules, we set the overall score to 100. This is a
  // hack to work around the fact that scores are computed before we
  // filter. See
  // http://code.google.com/p/page-speed/issues/detail?id=476 for the
  // relevant bug.
  bool has_any_results = false;
  for (int i = 0; i < formatted_results.rule_results_size(); ++i) {
    pagespeed::FormattedRuleResults* rule_results =
        formatted_results.mutable_rule_results(i);
    if (rule_results->url_blocks_size() == 0) {
      rule_results->set_rule_score(100);
      rule_results->set_rule_impact(0.0);
    } else {
      has_any_results = true;
    }
  }
  if (!has_any_results) {
    formatted_results.set_score(100);
  }

  // Convert the formatted results into JSON:
  scoped_ptr<base::Value> json_results(
      pagespeed::proto::FormattedResultsToJsonConverter::
      ConvertFormattedResults(formatted_results));
  if (json_results == NULL) {
    *error_string = "failed to ConvertFormattedResults";
    return false;
  }

  // Put optimized resources into JSON:
  scoped_ptr<base::DictionaryValue> optimized_content(
      new base::DictionaryValue);
  if (save_optimized_content) {
    SerializeOptimizedContent(filtered_results, optimized_content.get());
  }

  // Serialize all the JSON into a string.
  {
    scoped_ptr<base::DictionaryValue> root(new base::DictionaryValue);
    root->SetString("id", id);
    root->SetString("resourceFilterName", resource_filter_name);
    root->SetString("locale", locale);
    root->Set("results", json_results.release());
    root->Set("optimizedContent", optimized_content.release());
    base::JSONWriter::Write(root.get(), output_string);
  }

  return true;
}

}  // namespace pagespeed_chromium
