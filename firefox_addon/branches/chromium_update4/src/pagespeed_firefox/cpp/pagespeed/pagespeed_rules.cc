/**
 * Copyright 2011 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// Author: Matthew Steele

#include <stdio.h>
#include <string.h>
#include <string>
#include <vector>

#include "base/at_exit.h"
#include "base/basictypes.h"
#include "base/file_path.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"  // for base::JSONWriter::Write
#include "base/logging.h"
#include "base/md5.h"
#include "base/memory/scoped_ptr.h"
#include "base/string_number_conversions.h"  // for base::IntToString
#include "base/string_util.h"
#include "base/utf_string_conversions.h"
#include "base/values.h"
#include "build/build_config.h"
#include "googleurl/src/gurl.h"
#include "pagespeed/core/engine.h"
#include "pagespeed/core/file_util.h"
#include "pagespeed/core/pagespeed_init.h"
#include "pagespeed/core/pagespeed_input.h"
#include "pagespeed/core/serializer.h"
#include "pagespeed/dom/json_dom.h"
#include "pagespeed/filters/ad_filter.h"
#include "pagespeed/filters/landing_page_redirection_filter.h"
#include "pagespeed/filters/response_byte_result_filter.h"
#include "pagespeed/filters/tracker_filter.h"
#include "pagespeed/formatters/proto_formatter.h"
#include "pagespeed/har/http_archive.h"
#include "pagespeed/image_compression/image_attributes_factory.h"
#include "pagespeed/l10n/gettext_localizer.h"
#include "pagespeed/l10n/localizer.h"
#include "pagespeed/proto/formatted_results_to_json_converter.h"
#include "pagespeed/proto/pagespeed_output.pb.h"
#include "pagespeed/proto/pagespeed_proto_formatter.pb.h"
#include "pagespeed/proto/results_to_json_converter.h"
#include "pagespeed/rules/rule_provider.h"
#include "pagespeed_firefox/cpp/pagespeed/pagespeed_json_input.h"

#if defined(OS_WIN)
#define PlatformFOpen _wfopen
#else
#define PlatformFOpen fopen
#endif

namespace {

void Initialize() {
  static bool initialized = false;
  if (!initialized) {
    initialized = true;
#ifdef NDEBUG
    // In release builds, don't display INFO logs.
    logging::SetMinLogLevel(logging::LOG_WARNING);
#endif
    pagespeed::Init();
  }
}

// Copied from chromium net/base/net_util.cc to avoid pulling too much code from
// chromium net.

// what we prepend to get a file URL
static const FilePath::CharType kFileURLPrefix[] =
    FILE_PATH_LITERAL("file:///");

GURL FilePathToFileURL(const FilePath& path) {
  // Produce a URL like "file:///C:/foo" for a regular file, or
  // "file://///server/path" for UNC. The URL canonicalizer will fix up the
  // latter case to be the canonical UNC form: "file://server/path"
  FilePath::StringType url_string(kFileURLPrefix);
  url_string.append(path.value());

  // Now do replacement of some characters. Since we assume the input is a
  // literal filename, anything the URL parser might consider special should
  // be escaped here.

  // must be the first substitution since others will introduce percents as the
  // escape character
  ReplaceSubstringsAfterOffset(&url_string, 0,
      FILE_PATH_LITERAL("%"), FILE_PATH_LITERAL("%25"));

  // semicolon is supposed to be some kind of separator according to RFC 2396
  ReplaceSubstringsAfterOffset(&url_string, 0,
      FILE_PATH_LITERAL(";"), FILE_PATH_LITERAL("%3B"));

  ReplaceSubstringsAfterOffset(&url_string, 0,
      FILE_PATH_LITERAL("#"), FILE_PATH_LITERAL("%23"));

#if defined(OS_POSIX)
  ReplaceSubstringsAfterOffset(&url_string, 0,
      FILE_PATH_LITERAL("\\"), FILE_PATH_LITERAL("%5C"));
#endif

  return GURL(url_string);
}

class PluginSerializer : public pagespeed::Serializer {
 public:
  explicit PluginSerializer(const std::string& base_dir)
      : base_dir_(base_dir) {}
  virtual ~PluginSerializer() {}

  virtual std::string SerializeToFile(const std::string& content_url,
                                      const std::string& mime_type,
                                      const std::string& body);

 private:
  bool CreateFileForResource(const std::string& content_url,
                             const std::string& mime_type,
                             const std::string& body,
                             FilePath* file_path);

  const std::string base_dir_;

  DISALLOW_COPY_AND_ASSIGN(PluginSerializer);
};

std::string PluginSerializer::SerializeToFile(const std::string& content_url,
                                              const std::string& mime_type,
                                              const std::string& body) {
  FilePath file_path;
  if (!CreateFileForResource(
          content_url, mime_type, body, &file_path) ||
      file_path.empty()) {
    LOG(ERROR) << "Failed to CreateFileForResource for " << content_url;
    return "";
  }
  GURL url = FilePathToFileURL(file_path);
  FilePath::StringType string_path = file_path.value();

  // TODO(lsong): Use file_util to write file if possible.
  // Determine if the file exists.
  FILE* file = PlatformFOpen(string_path.c_str(), FILE_PATH_LITERAL("r"));
  if (file != NULL) {
    fclose(file);
    // Already exists. Since the path contains a hash of the contents,
    // assume the file on disk is the same as what we want to write,
    // and return the path of the file.
    return url.spec();
  }

  file = PlatformFOpen(string_path.c_str(), FILE_PATH_LITERAL("wb"));
  if (file == NULL) {
    LOG(ERROR) << "Unable to create file " << string_path;
    return "";
  }

  size_t num_written = fwrite(body.data(), 1, body.size(), file);
  fclose(file);
  if (num_written != body.size()) {
    LOG(ERROR) << "Failed to WriteDataToFile for " << string_path
               << " size=" << body.size() << " written=" << num_written;
    return "";
  }

  return url.spec();
}

bool PluginSerializer::CreateFileForResource(const std::string& content_url,
                                             const std::string& mime_type,
                                             const std::string& body,
                                             FilePath* file_path) {
  if (base_dir_.empty()) {
    LOG(DFATAL) << "No base directory available.";
    return false;
  }

  const GURL url(content_url);
  if (!url.is_valid()) {
    LOG(ERROR) << "Invalid url: " << content_url;
    return false;
  }

  const std::string filename =
      pagespeed::ChooseOutputFilename(url, mime_type, MD5String(body));
#if defined(OS_WIN)
  std::wstring w_base_dir;
  if (!UTF8ToWide(base_dir_.c_str(), base_dir_.size(), &w_base_dir)) {
    LOG(WARNING) << "Convert UTF8 to wstring is not 100% valid.";
  }
  FilePath base_dir(w_base_dir);
  std::wstring w_filename;
  if (!UTF8ToWide(filename.c_str(), filename.size(), &w_filename)) {
    LOG(WARNING) << "Convert UTF8 to wstring is not 100% valid.";
  }
  *file_path = base_dir.Append(w_filename);
#elif defined(OS_POSIX)
  FilePath base_dir(base_dir_);
  *file_path = base_dir.Append(filename);
#endif

  return true;
}

// Must be kept in sync with pagespeedLibraryRules.js filterChoice().
enum ResourceFilterEnum {
  RESOURCE_FILTER_ALL = 0,
  RESOURCE_FILTER_ONLY_ADS = 1,
  RESOURCE_FILTER_ONLY_TRACKERS = 2,
  RESOURCE_FILTER_ONLY_CONTENT = 3,
};

// Convert the filter choice passed to ComputeAndFormatResults to a
// ResourceFilter.  This routine must be kept in sync with
// js/pagespeed/pagespeedLibraryRules.js::filterChoice().
pagespeed::ResourceFilter* ChoiceToFilter(int filter_choice) {
  switch (filter_choice) {
    case RESOURCE_FILTER_ONLY_ADS:
      return new pagespeed::NotResourceFilter(new pagespeed::AdFilter());
    case RESOURCE_FILTER_ONLY_TRACKERS:
      return new pagespeed::NotResourceFilter(new pagespeed::TrackerFilter());
    case RESOURCE_FILTER_ONLY_CONTENT:
      return new pagespeed::AndResourceFilter(new pagespeed::AdFilter(),
                                              new pagespeed::TrackerFilter());
    default:
      LOG(ERROR) << "Unknown filter chioce " << filter_choice;
      // Intentional fall-through to allow all filter
    case RESOURCE_FILTER_ALL:
      return new pagespeed::AllowAllResourceFilter();
  }
}

pagespeed::PagespeedInput* ConstructPageSpeedInput(const char* har_data,
                                                   const char* custom_data,
                                                   const char* root_url,
                                                   const char* json_dom,
                                                   int filter_choice)  {
  scoped_ptr<pagespeed::PagespeedInput> input(
      pagespeed::ParseHttpArchiveWithFilter(
          har_data, ChoiceToFilter(filter_choice)));
  if (input == NULL) {
    return NULL;
  }

  if (!pagespeed::PopulateInputFromJSON(input.get(), custom_data)) {
    LOG(ERROR) << "Failed to parse custom JSON.";
    return NULL;
  }
  const std::string root_url_str(root_url);
  if (!root_url_str.empty()) {
    input->SetPrimaryResourceUrl(root_url_str);
  }
  std::string error_msg_out;
  scoped_ptr<const Value> document_json(base::JSONReader::ReadAndReturnError(
      json_dom,
      true,  // allow_trailing_comma
      NULL,  // error_code_out (ReadAndReturnError permits NULL here)
      &error_msg_out));
  if (document_json == NULL) {
    LOG(ERROR) << "Failed to parse document JSON." << error_msg_out;
    return NULL;
  }
  if (!document_json->IsType(Value::TYPE_DICTIONARY)) {
    LOG(ERROR) << "DOM must be a JSON dictionary";
    return NULL;
  }
  input->AcquireDomDocument(pagespeed::dom::CreateDocument(
      static_cast<const DictionaryValue*>(document_json.release())));
  input->AcquireImageAttributesFactory(
      new pagespeed::image_compression::ImageAttributesFactory());
  input->Freeze();
  return input.release();
}

void InstantiatePageSpeedRules(const pagespeed::PagespeedInput& input,
                               std::vector<pagespeed::Rule*>* rules) {
  const bool save_optimized_content = true;
  std::vector<std::string> incompatible_rule_names;
  pagespeed::rule_provider::AppendPageSpeedRules(save_optimized_content,
                                                 rules);
  pagespeed::rule_provider::RemoveIncompatibleRules(
      rules, &incompatible_rule_names, input.EstimateCapabilities());
  if (!incompatible_rule_names.empty()) {
    // We would like to display the rule names using base::JoinString,
    // however including base/string_util.h causes a collision with
    // mozilla headers, so we just print the number of excluded rules
    // instead.
    LOG(INFO) << "Removing " << incompatible_rule_names.size()
              << " incompatible rules.";
  }
}

// Copies the contents of a string to a malloc-allocated buffer and
// returns it. The caller is responsible for freeing returned memory.
const char* MallocString(const std::string& output_string) {
  const size_t length = output_string.length();
  char* buf = static_cast<char*>(malloc(length + 1));
  buf[length] = 0;
  memcpy(buf, output_string.c_str(), length);
  return buf;
}

}  // namespace

extern "C" {
// Export our public functions so Firefox is able to load us.
// See http://gcc.gnu.org/wiki/Visibility for more information.
#if defined(__GNUC__)
#pragma GCC visibility push(default)
#endif

// Caller is responsible for freeing returned memory. JS callers can
// accomplish this by calling PageSpeed_DoFree.
const char* PageSpeed_ComputeResults(const char* har_data,
                                     const char* custom_data,
                                     const char* root_url,
                                     const char* json_dom,
                                     int filter_choice) {
  Initialize();

  // Instantiate an AtExitManager so our Singleton<>s are able to
  // schedule themselves for destruction.
  base::AtExitManager at_exit_manager;

  scoped_ptr<pagespeed::PagespeedInput> input(
      ConstructPageSpeedInput(har_data,
                              custom_data,
                              root_url,
                              json_dom,
                              filter_choice));
  if (input == NULL) {
    return NULL;
  }

  std::vector<pagespeed::Rule*> rules;
  InstantiatePageSpeedRules(*input, &rules);

  // Ownership of rules is transferred to engine.
  pagespeed::Engine engine(&rules);
  engine.Init();

  pagespeed::Results results;
  engine.ComputeResults(*input, &results);

  std::string results_json;
  if (!pagespeed::proto::ResultsToJsonConverter::Convert(
          results, &results_json)) {
    LOG(ERROR) << "Failed to ResultsToJsonConverter::Convert.";
    return NULL;
  }

  return MallocString(results_json);
}

// Caller is responsible for freeing returned memory. JS callers can
// accomplish this by calling PageSpeed_DoFree.
const char* PageSpeed_ComputeAndFormatResults(const char* locale,
                                              const char* har_data,
                                              const char* custom_data,
                                              const char* root_url,
                                              const char* json_dom,
                                              int filter_choice,
                                              const char* output_dir) {
  Initialize();

  // Instantiate an AtExitManager so our Singleton<>s are able to
  // schedule themselves for destruction.
  base::AtExitManager at_exit_manager;

  scoped_ptr<pagespeed::PagespeedInput> input(
      ConstructPageSpeedInput(har_data,
                              custom_data,
                              root_url,
                              json_dom,
                              filter_choice));
  if (input == NULL) {
    return NULL;
  }

  // Create a localizer.
  scoped_ptr<pagespeed::l10n::Localizer> localizer(
      pagespeed::l10n::GettextLocalizer::Create(locale));
  if (localizer.get() == NULL) {
    LOG(WARNING) << "Could not create GettextLocalizer for locale: "
                 << locale;
    localizer.reset(new pagespeed::l10n::BasicLocalizer);
  }

  // Compute and format the results.  Keep the Results around so that we can
  // serialize optimized content.
  pagespeed::Results filtered_results;
  pagespeed::FormattedResults formatted_results;
  {
    std::vector<pagespeed::Rule*> rules;
    InstantiatePageSpeedRules(*input, &rules);

    // Ownership of rules is transferred to engine.
    pagespeed::Engine engine(&rules);
    engine.Init();

    pagespeed::Results unfiltered_results;
    engine.ComputeResults(*input, &unfiltered_results);
    // Filter the landing page redirection result, so that we do not flag
    // redirection from foo.com to www.foo.com.
    pagespeed::LandingPageRedirectionFilter redirection_filter;
    engine.FilterResults(unfiltered_results, redirection_filter,
                         &filtered_results);

    formatted_results.set_locale(localizer->GetLocale());
    pagespeed::formatters::ProtoFormatter formatter(
        localizer.get(), &formatted_results);

    // Filter the results (matching the code in Page Speed Online).
    pagespeed::ResponseByteResultFilter result_filter;
    if (!engine.FormatResults(filtered_results, result_filter, &formatter)) {
      LOG(ERROR) << "error formatting results in locale: " << locale;
      return NULL;
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
  }

  // Convert the formatted results into JSON:
  scoped_ptr<Value> json_results(
      pagespeed::proto::FormattedResultsToJsonConverter::
      ConvertFormattedResults(formatted_results));
  if (json_results == NULL) {
    LOG(ERROR) << "Failed to ConvertFormattedResults";
    return NULL;
  }

  // Serialize optimized resources to disk:
  scoped_ptr<DictionaryValue> paths(new DictionaryValue);
  if (output_dir) {
    PluginSerializer serializer(output_dir);
    for (int i = 0; i < filtered_results.rule_results_size(); ++i) {
      const pagespeed::RuleResults& rule_results =
          filtered_results.rule_results(i);
      for (int j = 0; j < rule_results.results_size(); ++j) {
        const pagespeed::Result& result = rule_results.results(j);
        if (result.has_optimized_content() && result.resource_urls_size() > 0) {
          const std::string key = base::IntToString(result.id());
          if (paths->HasKey(key)) {
            LOG(ERROR) << "Duplicate result id " << key;
          } else {
            paths->SetString(key, serializer.SerializeToFile(
                result.resource_urls(0),
                result.optimized_content_mime_type(),
                result.optimized_content()));
          }
        }
      }
    }
  }

  // Serialize all the JSON into a string.
  std::string output_string;
  {
    scoped_ptr<DictionaryValue> root(new DictionaryValue);
    root->Set("results", json_results.release());
    root->Set("optimized_content", paths.release());
    base::JSONWriter::Write(root.get(), false, &output_string);
  }

  return MallocString(output_string);
}

// Helper that exposes the capability to free memory to JS callers.
void PageSpeed_DoFree(char* mem) {
  free(mem);
}

#if defined(__GNUC__)
#pragma GCC visibility pop
#endif
}  // extern "C"
