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

#include "pagespeed_rules.h"

#include <fstream>
#include <sstream>
#include <vector>

#include "nsArrayUtils.h" // for do_QueryElementAt
#include "nsComponentManagerUtils.h" // for do_CreateInstance
#include "nsCOMPtr.h"
#include "nsIInputStream.h"
#include "nsIIOService.h"
#include "nsILocalFile.h"
#include "nsIURI.h"
#include "nsNetCID.h" // for NS_IOSERVICE_CONTRACTID
#include "nsServiceManagerUtils.h" // for do_GetService
#include "nsStringAPI.h"

#include "base/basictypes.h"
#include "base/logging.h"
#include "base/md5.h"
#include "file_util.h"
#include "firefox_dom.h"
#include "googleurl/src/gurl.h"
#include "pagespeed_json_input.h"
#include "pagespeed/core/engine.h"
#include "pagespeed/core/pagespeed_input.h"
#include "pagespeed/core/serializer.h"
#include "pagespeed/filters/ad_filter.h"
#include "pagespeed/formatters/json_formatter.h"
#include "pagespeed/rules/minify_css.h"
#include "pagespeed/rules/minify_html.h"
#include "pagespeed/rules/optimize_images.h"
#include "pagespeed/rules/rule_provider.h"

namespace {

class PluginSerializer : public pagespeed::Serializer {
 public:
  explicit PluginSerializer(nsILocalFile* base_dir)
      : base_dir_(base_dir) {}
  virtual ~PluginSerializer() {}

  virtual std::string SerializeToFile(const std::string& content_url,
                                      const std::string& mime_type,
                                      const std::string& body);

 private:
  nsCOMPtr<nsILocalFile> base_dir_;

  DISALLOW_COPY_AND_ASSIGN(PluginSerializer);
};

std::string PluginSerializer::SerializeToFile(const std::string& content_url,
                                              const std::string& mime_type,
                                              const std::string& body) {
  // Make a copy of the base_dir_ nsIFile object.
  nsCOMPtr<nsIFile> file;
  base_dir_->Clone(getter_AddRefs(file));
  if (NULL == file) {
    LOG(ERROR) << "Unable to clone nsILocalFile";
    return "";
  }

  // Choose a filename for the saved data.
  const GURL url(content_url);
  if (!url.is_valid()) {
    LOG(ERROR) << "Invalid url: " << content_url;
    return "";
  }
  const std::string filename =
      pagespeed::ChooseOutputFilename(url, mime_type, MD5String(body));
  file->Append(NS_ConvertASCIItoUTF16(filename.c_str()));

  // Get the absolute path of the nsIFile as a C++ string.
  nsString nsString_path;
  file->GetPath(nsString_path);
  const NS_ConvertUTF16toUTF8 utf8_path(nsString_path);
  const std::string string_path(utf8_path.get());

  // Write the data to the file.
  std::ofstream out(string_path.c_str(), std::ios::out | std::ios::binary);
  if (!out) {
    LOG(ERROR) << "Unable to save to file: " << string_path;
    return "";
  }
  out.write(body.c_str(), body.size());
  out.close();

  // Get the file URI for the nsIFile where we stored the data.
  nsCOMPtr<nsIIOService> io_service(do_GetService(NS_IOSERVICE_CONTRACTID));
  if (io_service == NULL) {
    LOG(ERROR) << "Unable to get nsIIOService";
    return "";
  }
  nsCOMPtr<nsIURI> uri;
  io_service->NewFileURI(file, getter_AddRefs(uri));
  if (uri == NULL) {
    LOG(ERROR) << "Unable to get file URI for path: " << string_path;
    return "";
  }
  nsCString uri_spec;
  uri->GetSpec(uri_spec);
  return uri_spec.get();
}

void AppendInputStreamsContents(nsIArray *input_streams,
                                std::vector<std::string>* contents) {
  if (input_streams != NULL) {
    PRUint32 length;
    input_streams->GetLength(&length);
    for (PRUint32 i = 0; i < length; ++i) {
      nsCOMPtr<nsIInputStream> input_stream(
          do_QueryElementAt(input_streams, i));
      std::string content;
      if (input_stream != NULL) {
        PRUint32 bytes_read = 0;
        do {
          char buffer[1024];
          input_stream->Read(buffer, arraysize(buffer), &bytes_read);
          content.append(buffer, static_cast<size_t>(bytes_read));
        } while (bytes_read > 0);
      }
      contents->push_back(content);
    }
  }
}

// Convert the filter choice passed to ComputeAndFormatResults to a
// ResourceFilter.  This routine must be kept in sync with
// js/pagespeed/pagespeedLibraryRules.js::filterChoice().
pagespeed::ResourceFilter* ChoiceToFilter(int filter_choice) {
  switch (filter_choice) {
    case IPageSpeedRules::RESOURCE_FILTER_ONLY_ADS:
      return new pagespeed::NotResourceFilter(new pagespeed::AdFilter());
    case IPageSpeedRules::RESOURCE_FILTER_EXCLUDE_ADS:
      return new pagespeed::AdFilter();
    default:
      LOG(ERROR) << "Unknown filter chioce " << filter_choice;
      // Intentional fall-through to allow all filter
    case IPageSpeedRules::RESOURCE_FILTER_ALL:
      return new pagespeed::AllowAllResourceFilter();
  }
}

}  // namespace

namespace pagespeed {

NS_IMPL_ISUPPORTS1(PageSpeedRules, IPageSpeedRules)

PageSpeedRules::PageSpeedRules() {}

PageSpeedRules::~PageSpeedRules() {}

NS_IMETHODIMP
PageSpeedRules::ComputeAndFormatResults(const char* data,
                                        nsIArray* input_streams,
                                        nsIDOMDocument* root_document,
                                        PRInt16 filter_choice,
                                        nsILocalFile* output_dir,
                                        char** _retval) {
  std::vector<std::string> contents;
  AppendInputStreamsContents(input_streams, &contents);

  const bool save_optimized_content = true;
  std::vector<Rule*> rules;
  rule_provider::AppendAllRules(save_optimized_content, &rules);

  Engine engine(&rules);  // Ownership of rules is transferred to engine.
  engine.Init();

  PagespeedInput input(ChoiceToFilter(filter_choice));
  input.AcquireDomDocument(new FirefoxDocument(root_document));
  if (PopulateInputFromJSON(&input, data, contents)) {
    std::stringstream stream;
    PluginSerializer serializer(output_dir);
    formatters::JsonFormatter formatter(&stream, &serializer);
    engine.ComputeAndFormatResults(input, &formatter);

    const std::string& output_string = stream.str();
    nsCString retval(output_string.c_str(), output_string.length());
    *_retval = NS_CStringCloneData(retval);
    return NS_OK;
  } else {
    return NS_ERROR_FAILURE;
  }
}

}  // namespace pagespeed
