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

#ifdef _WINDOWS
#include <windows.h>
#endif

#include "third_party/npapi/npapi.h"
#include "third_party/npapi/npfunctions.h"

#include "base/at_exit.h"
#include "base/base64.h"
#include "base/basictypes.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/logging.h"
#include "base/md5.h"
#include "base/scoped_ptr.h"
#include "base/stl_util-inl.h"
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
#include "pagespeed/timeline/json_importer.h"
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
#include "pagespeed/proto/timeline.pb.h"
#include "pagespeed/rules/rule_provider.h"
#include "pagespeed_chromium/json_dom.h"

namespace {

// These are the method names as JavaScript sees them:
const char* kPingMethodId = "ping";
const char* kRunPageSpeedMethodId = "runPageSpeed";

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
                               DictionaryValue* optimized_content) {
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
      scoped_ptr<DictionaryValue> entry(new DictionaryValue());
      entry->SetString("filename", pagespeed::ChooseOutputFilename(
          gurl, mimetype, MD5String(content)));
      entry->SetString("mimetype", mimetype);
      entry->SetString("content", encoded);
      optimized_content->Set(key, entry.release());
    }
  }
}

// Parse the HAR data and run the Page Speed rules, then format the results.
// Return false if the HAR data could not be parsed, true otherwise.
// This function will take ownership of the filter and document arguments, and
// will delete them before returning.
bool RunPageSpeedRules(const std::string& har_data,
                       pagespeed::DomDocument* document,
                       pagespeed::InstrumentationDataVector* timeline_events,
                       pagespeed::ResourceFilter* filter,
                       const std::string& locale,
                       bool save_optimized_content,
                       bool is_mobile,
                       std::string* output_string,
                       std::string* error_string) {
  // Instantiate an AtExitManager so our Singleton<>s are able to
  // schedule themselves for destruction.
  base::AtExitManager at_exit_manager;

  // Parse the HAR into a PagespeedInput object.  ParseHttpArchiveWithFilter
  // will ensure that filter gets deleted.
  scoped_ptr<pagespeed::PagespeedInput> input(
      pagespeed::ParseHttpArchiveWithFilter(har_data, filter));
  if (input.get() == NULL) {
    delete document;
    *error_string = "could not parse HAR";
    return false;
  }

  // Add the DOM document to the PagespeedInput object.
  if (document != NULL) {
    input->SetPrimaryResourceUrl(document->GetDocumentUrl());
  }
  input->AcquireDomDocument(document); // input takes ownership of document

  if (is_mobile) {
    pagespeed::ClientCharacteristics cc;
    pagespeed::pagespeed_input_util::PopulateMobileClientCharacteristics(&cc);
    input->SetClientCharacteristics(cc);
  }

  // Finish up the PagespeedInput object and freeze it.
  input->AcquireInstrumentationData(timeline_events);
  input->AcquireImageAttributesFactory(
      new pagespeed::image_compression::ImageAttributesFactory());
  input->Freeze();

  std::vector<pagespeed::Rule*> rules;

  // In environments where exceptions can be thrown, use
  // STLElementDeleter to make sure we free the rules in the event
  // that they are not transferred to the Engine.
  STLElementDeleter<std::vector<pagespeed::Rule*> > rule_deleter(&rules);

  pagespeed::rule_provider::AppendPageSpeedRules(
      save_optimized_content, &rules);
  if (is_mobile) {
    pagespeed::rule_provider::AppendRuleSet(
        save_optimized_content,
        pagespeed::rule_provider::MOBILE_BROWSER_RULES,
        &rules);
  }
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
  pagespeed::Results results;
  if (!engine.ComputeResults(*input, &results)) {
    std::vector<std::string> error_rules;
    for (int i = 0, size = results.error_rules_size(); i < size; ++i) {
      error_rules.push_back(results.error_rules(i));
    }
    LOG(WARNING) << "Errors during ComputeResults in rules: "
                 << JoinString(error_rules, ' ');
  }

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
    if (!engine.FormatResults(results, result_filter, &formatter)) {
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
  scoped_ptr<Value> json_results(
      pagespeed::proto::FormattedResultsToJsonConverter::
      ConvertFormattedResults(formatted_results));
  if (json_results == NULL) {
    *error_string = "failed to ConvertFormattedResults";
    return false;
  }

  // Put optimized resources into JSON:
  scoped_ptr<DictionaryValue> optimized_content(new DictionaryValue);
  if (save_optimized_content) {
    SerializeOptimizedContent(results, optimized_content.get());
  }

  // Serialize all the JSON into a string.
  {
    scoped_ptr<DictionaryValue> root(new DictionaryValue);
    root->Set("results", json_results.release());
    root->Set("optimizedContent", optimized_content.release());
    base::JSONWriter::Write(root.get(), false, output_string);
  }

  return true;
}

class PageSpeedModule : public NPObject {
 public:
  explicit PageSpeedModule(NPP npp) : npp_(npp) {}

  // Run the Page Speed library, given a Javascript reference to the DOM
  // document (or null) and a Javascript string indicating what filter to use
  // ("ads", "trackers", "content", or "all").  Returns JSON results (as a
  // string) to the Javascript caller.
  bool RunPageSpeed(const NPVariant& har_string,
                    const NPVariant& dom_document,
                    const NPVariant& timeline_events,
                    const NPVariant& filter_name,
                    const NPVariant& locale_string,
                    const NPVariant& save_optimized_content,
                    const NPVariant& is_mobile,
                    NPVariant *result);

  // Indicate that a Javascript exception should be thrown, and return a bool
  // that can be used as a return value for Invoke().
  bool Throw(const std::string& message);

 private:
  // An NPP is a handle to an NPAPI plugin, and we need it to be able to call
  // out to Javascript via functions like NPN_GetProperty().  We keep it here
  // so we can pass it to ChromiumDocument objects we create, so that those
  // objects can call out to Javascript to inspect the DOM.
  const NPP npp_;

  DISALLOW_COPY_AND_ASSIGN(PageSpeedModule);
};

// Run Page Speed on the contents of the input buffer, and put the results into
// the output buffer.
bool PageSpeedModule::RunPageSpeed(const NPVariant& har_arg,
                                   const NPVariant& document_arg,
                                   const NPVariant& timeline_arg,
                                   const NPVariant& filter_arg,
                                   const NPVariant& locale_arg,
                                   const NPVariant& save_optimized_content_arg,
                                   const NPVariant& is_mobile_arg,
                                   NPVariant *result) {
  if (!NPVARIANT_IS_STRING(har_arg)) {
    return Throw("first argument to runPageSpeed must be a string");
  }
  if (!NPVARIANT_IS_STRING(document_arg)) {
    return Throw("second argument to runPageSpeed must be a string");
  }
  if (!NPVARIANT_IS_STRING(timeline_arg)) {
    return Throw("third argument to runPageSpeed must be a string");
  }
  if (!NPVARIANT_IS_STRING(filter_arg)) {
    return Throw("fourth argument to runPageSpeed must be a string");
  }
  if (!NPVARIANT_IS_STRING(locale_arg)) {
    return Throw("fifth argument to runPageSpeed must be a string");
  }
  if (!NPVARIANT_IS_BOOLEAN(save_optimized_content_arg)) {
    return Throw("sixth argument to runPageSpeed must be a boolean");
  }
  if (!NPVARIANT_IS_BOOLEAN(is_mobile_arg)) {
    return Throw("seventh argument to runPageSpeed must be a boolean");
  }

  const NPString& har_NPString = NPVARIANT_TO_STRING(har_arg);
  const std::string har_string(har_NPString.UTF8Characters,
                               har_NPString.UTF8Length);

  const NPString& document_NPString = NPVARIANT_TO_STRING(document_arg);
  const std::string document_string(document_NPString.UTF8Characters,
                                    document_NPString.UTF8Length);

  const NPString& timeline_NPString = NPVARIANT_TO_STRING(timeline_arg);
  const std::string timeline_string(timeline_NPString.UTF8Characters,
                                    timeline_NPString.UTF8Length);

  const NPString& filter_NPString = NPVARIANT_TO_STRING(filter_arg);
  const std::string filter_string(filter_NPString.UTF8Characters,
                                  filter_NPString.UTF8Length);

  const NPString& locale_NPString = NPVARIANT_TO_STRING(locale_arg);
  const std::string locale_string(locale_NPString.UTF8Characters,
                                  locale_NPString.UTF8Length);

  const bool save_optimized_content =
      NPVARIANT_TO_BOOLEAN(save_optimized_content_arg);

  const bool is_mobile = NPVARIANT_TO_BOOLEAN(is_mobile_arg);

  std::string error_msg_out;
  scoped_ptr<const Value> document_json(base::JSONReader::ReadAndReturnError(
      document_string,
      true,  // allow_trailing_comma
      NULL,  // error_code_out (ReadAndReturnError permits NULL here)
      &error_msg_out));
  if (document_json == NULL) {
    return Throw("could not parse DOM: " + error_msg_out);
  }
  if (!document_json->IsType(Value::TYPE_DICTIONARY)) {
    return Throw("DOM must be a JSON dictionary");
  }
  // The document does _not_ get ownership of document_json.  The reason for
  // this design choice is that the Value objects for subdocuments are owned by
  // their parent Value objects, so in order to avoid a double-free, instances
  // of the JsonDocument class need to not own the Value objects on which
  // they're based.
  pagespeed::DomDocument* document = pagespeed_chromium::CreateDocument(
      static_cast<const DictionaryValue*>(document_json.get()));

  pagespeed::InstrumentationDataVector timeline_protos;
  STLElementDeleter<pagespeed::InstrumentationDataVector>
      timeline_deleter(&timeline_protos);
  if (!pagespeed::timeline::CreateTimelineProtoFromJsonString(
          timeline_string, &timeline_protos)) {
    return Throw("error in timeline data");
  }

  std::string output, error_string;
  // RunPageSpeedRules will deallocate the filter and the document.
  const bool success = RunPageSpeedRules(
      har_string, document, &timeline_protos, NewFilter(filter_string),
      locale_string, save_optimized_content, is_mobile, &output, &error_string);
  if (!success) {
    return Throw(error_string);
  }

  if (result) {
    const size_t data_length = output.size();
    char* data_copy = static_cast<char*>(npnfuncs->memalloc(data_length));
    memcpy(data_copy, output.data(), data_length);
    STRINGN_TO_NPVARIANT(data_copy, data_length, *result);
  }
  return true;
}

bool PageSpeedModule::Throw(const std::string& message) {
  LOG(ERROR) << "PageSpeedModule::Throw " << message;
  npnfuncs->setexception(this, message.c_str());
  // You'd think we'd want to return false, to indicate an error.  If we do
  // that, then Chrome will still throw a JS error, but it will use a generic
  // error message instead of the one given here.  Using true seems to work.
  return true;
}

NPObject* Allocate(NPP npp, NPClass* npclass) {
  return new PageSpeedModule(npp);
}

void Deallocate(NPObject* object) {
  delete object;
}

// Return true if method_name is a recognized method.
bool HasMethod(NPObject* obj, NPIdentifier method_name) {
  char *name = npnfuncs->utf8fromidentifier(method_name);
  bool has_method = false;
  if (!strcmp(name, kPingMethodId)) {
    has_method = true;
  } else if (!strcmp(name, kRunPageSpeedMethodId)) {
    has_method = true;
  }
  npnfuncs->memfree(name);
  return has_method;
}

// Called by the browser to invoke the default method on an NPObject.
// Returns null.
// Apparently the plugin won't load properly if we simply
// tell the browser we don't have this method.
// Called by NPN_InvokeDefault, declared in npruntime.h
// Documentation URL: https://developer.mozilla.org/en/NPClass
bool InvokeDefault(NPObject *obj, const NPVariant *args,
                   uint32_t argCount, NPVariant *result) {
  if (result) {
    NULL_TO_NPVARIANT(*result);
  }
  return true;
}

// Invoke() is called by the browser to invoke a function object whose name
// is method_name.
bool Invoke(NPObject* obj,
            NPIdentifier method_name,
            const NPVariant *args,
            uint32_t arg_count,
            NPVariant *result) {
  NULL_TO_NPVARIANT(*result);
  char *name = npnfuncs->utf8fromidentifier(method_name);
  if (name == NULL) {
    return false;
  }
  bool rval = false;
  PageSpeedModule* module = static_cast<PageSpeedModule*>(obj);
  // Map the method name to a function call.  |result| is filled in by the
  // called function, then gets returned to the browser when Invoke() returns.
  if (!strcmp(name, kPingMethodId)) {
    if (arg_count == 0) {
      if (result) {
        NULL_TO_NPVARIANT(*result);
      }
      rval = true;
    } else {
      rval = module->Throw("wrong number of arguments to ping");
    }
  } else if (!strcmp(name, kRunPageSpeedMethodId)) {
    if (arg_count == 7) {
      rval = module->RunPageSpeed(args[0], args[1], args[2], args[3], args[4],
                                  args[5], args[6], result);
    } else {
      rval = module->Throw("wrong number of arguments to runPageSpeed");
    }
  }
  // Since name was allocated above by npnfuncs->utf8fromidentifier,
  // it needs to be freed here.
  npnfuncs->memfree(name);
  return rval;
}

// The class structure that gets passed back to the browser.  This structure
// provides funtion pointers that the browser calls.
NPClass kPageSpeedClass = {
  NP_CLASS_STRUCT_VERSION,
  Allocate,
  Deallocate,
  NULL,  // Invalidate is not implemented
  HasMethod,
  Invoke,
  InvokeDefault,
  NULL,  // HasProperty is not implemented
  NULL,  // GetProperty is not implemented
  NULL,  // SetProperty is not implemented
};

}  // namespace

NPNetscapeFuncs* npnfuncs;

NPClass* GetNPSimpleClass() {
  return &kPageSpeedClass;
}

void InitializePageSpeedPlugin() {
#ifdef NDEBUG
  // In release builds, don't display INFO logs.
  logging::SetMinLogLevel(logging::LOG_WARNING);
#endif
  pagespeed::Init();
}

void ShutDownPageSpeedPlugin() {
  pagespeed::ShutDown();
}
