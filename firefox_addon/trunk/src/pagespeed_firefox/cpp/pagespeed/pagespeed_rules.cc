/**
 * Copyright 2009 Google Inc.
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

#include "pagespeed_firefox/cpp/pagespeed/pagespeed_rules.h"

#include <sstream>
#include <string>
#include <vector>

#include "nsCOMPtr.h"  // NOLINT
#include "nsIIOService.h"  // NOLINT
#include "nsILocalFile.h"  // NOLINT
#include "nsIURI.h"  // NOLINT
#include "nsNetCID.h"  // for NS_IOSERVICE_CONTRACTID  // NOLINT
#include "nsNetUtil.h"  // for NS_NewLocalFileOutputStream  // NOLINT
#include "nsServiceManagerUtils.h"  // for do_GetService  // NOLINT
#include "nsStringAPI.h"  // NOLINT

#include "base/at_exit.h"
#include "base/basictypes.h"
#include "base/json/json_writer.h"  // for base::JSONWriter::Write
#include "base/logging.h"
#include "base/md5.h"
#include "base/scoped_ptr.h"
#include "base/string_number_conversions.h"  // for base::IntToString
#include "base/values.h"
#include "googleurl/src/gurl.h"
#include "pagespeed/core/engine.h"
#include "pagespeed/core/file_util.h"
#include "pagespeed/core/pagespeed_init.h"
#include "pagespeed/core/pagespeed_input.h"
#include "pagespeed/core/serializer.h"
#include "pagespeed/filters/ad_filter.h"
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
#include "pagespeed_firefox/cpp/pagespeed/firefox_dom.h"
#include "pagespeed_firefox/cpp/pagespeed/pagespeed_json_input.h"

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

// Compute the URI spec for a given file.
bool ComputeUriSpec(nsIFile* file, nsCString* out_uri_spec) {
  nsCOMPtr<nsIIOService> io_service(do_GetService(NS_IOSERVICE_CONTRACTID));
  if (io_service == NULL) {
    LOG(ERROR) << "Unable to get nsIIOService";
    return false;
  }
  nsCOMPtr<nsIURI> uri;
  if (NS_FAILED(io_service->NewFileURI(file, getter_AddRefs(uri))) ||
      uri == NULL) {
    LOG(ERROR) << "Unable to get file URI.";
    return false;
  }
  if (NS_FAILED(uri->GetSpec(*out_uri_spec))) {
    LOG(ERROR) << "Unable to get file spec.";
    return false;
  }

  return true;
}

// Get the path for the nsIFile as a std::string.
std::string GetPathForFile(nsIFile* file) {
  nsString nsString_path;
  if (NS_FAILED(file->GetPath(nsString_path))) {
    LOG(ERROR) << "Failed to GetPath.";
    return "";
  }
  const NS_ConvertUTF16toUTF8 utf8_path(nsString_path);
  return utf8_path.get();
}

// Determine if the given file is a writable directory.
bool IsWritableDirectory(nsIFile* file) {
  PRBool out_is_directory = PR_FALSE;
  if (NS_FAILED(file->IsDirectory(&out_is_directory)) ||
      out_is_directory == PR_FALSE) {
    return false;
  }
  PRBool out_is_directory_writable = PR_FALSE;
  if (NS_FAILED(file->IsWritable(&out_is_directory_writable)) ||
      out_is_directory_writable == PR_FALSE) {
    return false;
  }

  return true;
}

// Write the data to the file.
bool WriteDataToFile(nsIFile* file, const std::string& body) {
  nsCOMPtr<nsIOutputStream> out;
  if (NS_FAILED(NS_NewLocalFileOutputStream(getter_AddRefs(out), file))) {
    LOG(ERROR) << "Failed to create output stream.";
    return false;
  }
  PRUint32 num_written = 0;
  if (NS_FAILED(out->Write(body.c_str(), body.size(), &num_written)) ||
      num_written != body.size()) {
    LOG(ERROR) << "Failed to write to file.";
    return false;
  }
  if (NS_FAILED(out->Close())) {
    LOG(ERROR) << "Failed to close file.";
    return false;
  }

  return true;
}

class PluginSerializer : public pagespeed::Serializer {
 public:
  explicit PluginSerializer(nsILocalFile* base_dir)
      : base_dir_(base_dir) {}
  virtual ~PluginSerializer() {}

  virtual std::string SerializeToFile(const std::string& content_url,
                                      const std::string& mime_type,
                                      const std::string& body);

 private:
  bool CreateFileForResource(const std::string& content_url,
                             const std::string& mime_type,
                             const std::string& body,
                             nsIFile** file);

  nsCOMPtr<nsILocalFile> base_dir_;

  DISALLOW_COPY_AND_ASSIGN(PluginSerializer);
};

std::string PluginSerializer::SerializeToFile(const std::string& content_url,
                                              const std::string& mime_type,
                                              const std::string& body) {
  nsCOMPtr<nsIFile> file;
  if (!CreateFileForResource(
          content_url, mime_type, body, getter_AddRefs(file)) ||
      file == NULL) {
    LOG(ERROR) << "Failed to CreateFileForResource for " << content_url;
    return "";
  }

  // Compute the path to the file as a std::string. Used for debugging
  // only.
  std::string string_path(GetPathForFile(file));

  // Determine if the file exists.
  PRBool out_file_exists = PR_FALSE;
  if (NS_FAILED(file->Exists(&out_file_exists))) {
    LOG(ERROR) << "Unable to determine if file exists: " << string_path;
    return "";
  }

  // Get the file URI for the nsIFile where we stored the data.
  nsCString uri_spec;
  if (!ComputeUriSpec(file, &uri_spec)) {
    LOG(ERROR) << "Unable to ComputeUriSpec for " << string_path;
    return "";
  }

  if (out_file_exists == PR_TRUE) {
    // Already exists. Since the path contains a hash of the contents,
    // assume the file on disk is the same as what we want to write,
    // and return the path of the file.
    return uri_spec.get();
  }

  if (!IsWritableDirectory(base_dir_)) {
    LOG(ERROR) << "Unable to write to non-writable directory.";
    return "";
  }

  // Attempt to create the file with appropriate permissions.
  if (NS_FAILED(file->Create(nsIFile::NORMAL_FILE_TYPE, 0600))) {
    LOG(ERROR) << "Unable to create file " << string_path;
    return "";
  }

  PRBool out_is_file_writable = PR_FALSE;
  if (NS_FAILED(file->IsWritable(&out_is_file_writable)) ||
      out_is_file_writable == PR_FALSE) {
    LOG(ERROR) << "Unable to write to non-writable file " << string_path;
    return "";
  }

  if (!WriteDataToFile(file, body)) {
    LOG(ERROR) << "Failed to WriteDataToFile for " << string_path;
    return false;
  }

  return uri_spec.get();
}

bool PluginSerializer::CreateFileForResource(const std::string& content_url,
                                             const std::string& mime_type,
                                             const std::string& body,
                                             nsIFile** out_file) {
  if (base_dir_ == NULL) {
    LOG(DFATAL) << "No base directory available.";
    return false;
  }

  const GURL url(content_url);
  if (!url.is_valid()) {
    LOG(ERROR) << "Invalid url: " << content_url;
    return false;
  }
  // Make a copy of the base_dir_.
  nsCOMPtr<nsIFile> file;
  if (NS_FAILED(base_dir_->Clone(getter_AddRefs(file))) || NULL == file) {
    LOG(ERROR) << "Unable to clone directory.";
    return false;
  }
  const std::string filename =
      pagespeed::ChooseOutputFilename(url, mime_type, MD5String(body));
  if (NS_FAILED(file->Append(NS_ConvertASCIItoUTF16(filename.c_str())))) {
    LOG(ERROR) << "Failed to nsILocalFile::Append " << filename;
    return false;
  }

  *out_file = file;
  NS_ADDREF(*out_file);
  return true;
}

// Convert the filter choice passed to ComputeAndFormatResults to a
// ResourceFilter.  This routine must be kept in sync with
// js/pagespeed/pagespeedLibraryRules.js::filterChoice().
pagespeed::ResourceFilter* ChoiceToFilter(int filter_choice) {
  switch (filter_choice) {
    case IPageSpeedRules::RESOURCE_FILTER_ONLY_ADS:
      return new pagespeed::NotResourceFilter(new pagespeed::AdFilter());
    case IPageSpeedRules::RESOURCE_FILTER_ONLY_TRACKERS:
      return new pagespeed::NotResourceFilter(new pagespeed::TrackerFilter());
    case IPageSpeedRules::RESOURCE_FILTER_ONLY_CONTENT:
      return new pagespeed::AndResourceFilter(new pagespeed::AdFilter(),
                                              new pagespeed::TrackerFilter());
    default:
      LOG(ERROR) << "Unknown filter chioce " << filter_choice;
      // Intentional fall-through to allow all filter
    case IPageSpeedRules::RESOURCE_FILTER_ALL:
      return new pagespeed::AllowAllResourceFilter();
  }
}

// Helper that converts from mozilla nsACString to const char*.
bool GetDataFromCString(const nsACString& in,
                        const char** out_utf8) {
  PRBool terminated = PR_FALSE;
  NS_CStringGetData(in, out_utf8, &terminated);
  return terminated;
}

// Creates a PagespeedInput from the specified parameters.
pagespeed::PagespeedInput*
ConstructPageSpeedInput(const nsACString& har_data,
                        const nsACString& custom_data,
                        const nsACString& root_url,
                        nsIDOMDocument* root_document,
                        PRInt16 filter_choice) {
  const char* har_data_utf8;
  const char* custom_data_utf8;
  const char* root_url_utf8;
  if (!GetDataFromCString(har_data, &har_data_utf8) ||
      !GetDataFromCString(custom_data, &custom_data_utf8) ||
      !GetDataFromCString(root_url, &root_url_utf8)) {
    LOG(ERROR) << "Failed to convert strings to UTF8.";
    return NULL;
  }

  scoped_ptr<pagespeed::PagespeedInput> input(
      pagespeed::ParseHttpArchiveWithFilter(
          har_data_utf8, ChoiceToFilter(filter_choice)));
  if (input == NULL) {
    return NULL;
  }

  if (!pagespeed::PopulateInputFromJSON(input.get(), custom_data_utf8)) {
    LOG(ERROR) << "Failed to parse custom JSON.";
    return NULL;
  }
  const std::string root_url_str(root_url_utf8);
  if (!root_url_str.empty()) {
    input->SetPrimaryResourceUrl(root_url_str);
  }
  input->AcquireDomDocument(pagespeed::firefox::CreateDocument(root_document));
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

}  // namespace

namespace pagespeed {

NS_IMPL_ISUPPORTS1(PageSpeedRules, IPageSpeedRules)

PageSpeedRules::PageSpeedRules() {}

PageSpeedRules::~PageSpeedRules() {}

NS_IMETHODIMP
PageSpeedRules::ComputeResults(const nsACString& har_data,
                               const nsACString& custom_data,
                               const nsACString& root_url,
                               nsIDOMDocument* root_document,
                               PRInt16 filter_choice,
                               nsACString& _retval NS_OUTPARAM) {
  Initialize();

  // Instantiate an AtExitManager so our Singleton<>s are able to
  // schedule themselves for destruction.
  base::AtExitManager at_exit_manager;

  scoped_ptr<PagespeedInput> input(ConstructPageSpeedInput(har_data,
                                                           custom_data,
                                                           root_url,
                                                           root_document,
                                                           filter_choice));

  if (input == NULL) {
    LOG(ERROR) << "Failed to construct PagespeedInput.";
    return NS_ERROR_FAILURE;
  }

  std::vector<pagespeed::Rule*> rules;
  InstantiatePageSpeedRules(*input, &rules);

  Engine engine(&rules);  // Ownership of rules is transferred to engine.
  engine.Init();

  Results results;
  engine.ComputeResults(*input, &results);

  std::string results_json;
  if (!pagespeed::proto::ResultsToJsonConverter::Convert(
          results, &results_json)) {
    LOG(ERROR) << "Failed to ResultsToJsonConverter::Convert.";
    return NS_ERROR_FAILURE;
  }

  _retval.Assign(results_json.c_str(), results_json.length());
  return NS_OK;
}

NS_IMETHODIMP
PageSpeedRules::ComputeAndFormatResults(const nsACString& locale,
                                        const nsACString& har_data,
                                        const nsACString& custom_data,
                                        const nsACString& root_url,
                                        nsIDOMDocument* root_document,
                                        PRInt16 filter_choice,
                                        nsILocalFile* output_dir,
                                        nsACString& _retval NS_OUTPARAM) {
  Initialize();

  // Instantiate an AtExitManager so our Singleton<>s are able to
  // schedule themselves for destruction.
  base::AtExitManager at_exit_manager;

  const char* locale_utf8;
  if (!GetDataFromCString(locale, &locale_utf8)) {
    LOG(ERROR) << "Failed to convert locale string to UTF8.";
    return NS_ERROR_FAILURE;
  }

  scoped_ptr<PagespeedInput> input(ConstructPageSpeedInput(har_data,
                                                           custom_data,
                                                           root_url,
                                                           root_document,
                                                           filter_choice));

  if (input == NULL) {
    LOG(ERROR) << "Failed to construct PagespeedInput.";
    return NS_ERROR_FAILURE;
  }

  // Create a localizer.
  scoped_ptr<pagespeed::l10n::Localizer> localizer(
      pagespeed::l10n::GettextLocalizer::Create(locale_utf8));
  if (localizer.get() == NULL) {
    LOG(WARNING) << "Could not create GettextLocalizer for locale: "
                 << locale_utf8;
    localizer.reset(new pagespeed::l10n::BasicLocalizer);
  }

  // Compute and format the results.  Keep the Results around so that we can
  // serialize optimized content.
  Results results;
  FormattedResults formatted_results;
  {
    std::vector<pagespeed::Rule*> rules;
    InstantiatePageSpeedRules(*input, &rules);

    Engine engine(&rules);  // Ownership of rules is transferred to engine.
    engine.Init();

    engine.ComputeResults(*input, &results);

    formatted_results.set_locale(localizer->GetLocale());
    formatters::ProtoFormatter formatter(localizer.get(), &formatted_results);

    // Filter the results (matching the code in Page Speed Online).
    ResponseByteResultFilter result_filter;
    if (!engine.FormatResults(results, result_filter, &formatter)) {
      LOG(ERROR) << "error formatting results in locale: " << locale_utf8;
      return false;
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
      FormattedRuleResults* rule_results =
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
    return NS_ERROR_FAILURE;
  }

  // Serialize optimized resources to disk:
  scoped_ptr<DictionaryValue> paths(new DictionaryValue);
  PluginSerializer serializer(output_dir);
  for (int i = 0; i < results.rule_results_size(); ++i) {
    const RuleResults& rule_results = results.rule_results(i);
    for (int j = 0; j < rule_results.results_size(); ++j) {
      const Result& result = rule_results.results(j);
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

  // Serialize all the JSON into a string.
  std::string output_string;
  {
    scoped_ptr<DictionaryValue> root(new DictionaryValue);
    root->Set("results", json_results.release());
    root->Set("optimized_content", paths.release());
    base::JSONWriter::Write(root.get(), false, &output_string);
  }

  // Send the JSON output back to the front-end:
  _retval.Assign(output_string.c_str(), output_string.length());
  return NS_OK;
}

}  // namespace pagespeed
