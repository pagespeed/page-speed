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

#include <algorithm>  // for std::min()
#include <string>
#include <vector>

#ifdef _WINDOWS
#include <windows.h>
#endif

#include "third_party/npapi/npapi.h"
#include "third_party/npapi/npfunctions.h"

#include "base/at_exit.h"
#include "base/basictypes.h"
#include "base/logging.h"
#include "base/scoped_ptr.h"
#include "base/stl_util-inl.h"
#include "base/string_util.h"
#include "pagespeed/core/engine.h"
#include "pagespeed/core/formatter.h"
#include "pagespeed/core/pagespeed_input.h"
#include "pagespeed/core/resource_filter.h"
#include "pagespeed/core/rule.h"
#include "pagespeed/filters/ad_filter.h"
#include "pagespeed/filters/tracker_filter.h"
#include "pagespeed/formatters/proto_formatter.h"
#include "pagespeed/har/http_archive.h"
#include "pagespeed/image_compression/image_attributes_factory.h"
#include "pagespeed/l10n/localizer.h"
#include "pagespeed/proto/formatted_results_to_json_converter.h"
#include "pagespeed/proto/pagespeed_proto_formatter.pb.h"
#include "pagespeed/rules/rule_provider.h"
#include "pagespeed_chromium/npapi_dom.h"

extern NPNetscapeFuncs* npnfuncs;

namespace {

// These are the method names as JavaScript sees them:
const char* kAppendInputMethodId = "appendInput";
const char* kRunPageSpeedMethodId = "runPageSpeed";
const char* kReadMoreOutputMethodId = "readMoreOutput";

// How much output we send per call to getOutput():
const size_t kChunkSize = 8192;

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

// Parse the HAR data and run the Page Speed rules, then format the results.
// Return false if the HAR data could not be parsed, true otherwise.
// This function will take ownership of the filter and document arguments, and
// will delete them before returning.
bool RunPageSpeedRules(pagespeed::ResourceFilter* filter,
                       pagespeed::DomDocument* document,
                       const std::string& har_data,
                       std::string* output) {
#ifdef NDEBUG
  // In release builds, don't display INFO logs. Ideally we would do
  // this at process startup but we don't receive any native callbacks
  // at that point, so we do it here instead.
  logging::SetMinLogLevel(logging::LOG_WARNING);
#endif

  // Instantiate an AtExitManager so our Singleton<>s are able to
  // schedule themselves for destruction.
  base::AtExitManager at_exit_manager;

  // Parse the HAR into a PagespeedInput object.  ParseHttpArchiveWithFilter
  // will ensure that filter gets deleted.
  scoped_ptr<pagespeed::PagespeedInput> input(
      pagespeed::ParseHttpArchiveWithFilter(har_data, filter));
  if (input.get() == NULL) {
    delete document;
    return false;
  }

  // TODO: call input->SetPrimaryResourceUrl() if we can get the URL
  // from the HAR.

  input->AcquireDomDocument(document); // input takes ownership of document
  input->AcquireImageAttributesFactory(
      new pagespeed::image_compression::ImageAttributesFactory());
  input->Freeze();

  std::vector<pagespeed::Rule*> rules;

  // In environments where exceptions can be thrown, use
  // STLElementDeleter to make sure we free the rules in the event
  // that they are not transferred to the Engine.
  STLElementDeleter<std::vector<pagespeed::Rule*> > rule_deleter(&rules);

  const bool save_optimized_content = false;
  std::vector<std::string> incompatible_rule_names;
  pagespeed::rule_provider::AppendCompatibleRules(
      save_optimized_content,
      &rules,
      &incompatible_rule_names,
      input->EstimateCapabilities());
  if (!incompatible_rule_names.empty()) {
    std::string incompatible_rule_list =
        JoinString(incompatible_rule_names, ' ');
    LOG(INFO) << "Removing incompatible rules: " << incompatible_rule_list;
  }

  // Ownership of rules is transferred to the Engine instance.
  pagespeed::Engine engine(&rules);
  engine.Init();

  // Compute and format results.
  pagespeed::l10n::BasicLocalizer localizer;
  pagespeed::FormattedResults formatted_results;
  // TODO(mdsteele): Add an argument to support other locales.
  formatted_results.set_locale("en_US");
  pagespeed::formatters::ProtoFormatter formatter(&localizer,
                                                  &formatted_results);
  engine.ComputeAndFormatResults(*input, &formatter);

  pagespeed::proto::FormattedResultsToJsonConverter::Convert(
      formatted_results, output);

  return true;
}

class PageSpeedModule : public NPObject {
 public:
  explicit PageSpeedModule(NPP npp)
      : npp_(npp), output_start_(0) {}

  // Append the given Javascript string to our input buffer, and return null.
  bool AppendInput(const NPVariant& input_string, NPVariant *result);

  // Run the Page Speed library, given a Javascript reference to the DOM
  // document (or null) and a Javascript string indicating what filter to use
  // ("ads", "trackers", "content", or "all").  The current contents of the
  // input buffer are parsed as a HAR, then the input buffer is cleared and the
  // output buffer is populated with the JSON results from the library.
  // Returns null to the Javascript caller.
  bool RunPageSpeed(const NPVariant& dom_document,
                    const NPVariant& filter_name, NPVariant *result);

  // Return the next chunk of data from our output buffer as a Javascript
  // string, or return null if the output buffer is empty.
  bool ReadMoreOutput(NPVariant* result);

  // Indicate that a Javascript exception should be thrown, and return a bool
  // that can be used as a return value for Invoke().
  bool Throw(const std::string& message);

 private:
  // An NPP is a handle to an NPAPI plugin, and we need it to be able to call
  // out to Javascript via functions like NPN_GetProperty().  We keep it here
  // so we can pass it to ChromiumDocument objects we create, so that those
  // objects can call out to Javascript to inspect the DOM.
  const NPP npp_;

  // Buffers for input/output; data has to be transferred a piece at a time,
  // because SRPC currently can't handle strings larger than one or two dozen
  // kilobytes.
  std::string input_buffer_;
  std::string output_buffer_;
  size_t output_start_;

  DISALLOW_COPY_AND_ASSIGN(PageSpeedModule);
};

// Accept a chunk of data as a string, and append it to the input buffer.
bool PageSpeedModule::AppendInput(const NPVariant& argument,
                                  NPVariant *result) {
  if (!NPVARIANT_IS_STRING(argument)) {
    return Throw("appendInput expects a string argument");
  }
  const NPString& arg_NPString = NPVARIANT_TO_STRING(argument);
  input_buffer_.append(arg_NPString.UTF8Characters, arg_NPString.UTF8Length);
  if (result) {
    NULL_TO_NPVARIANT(*result);
  }
  return true;
}

// Run Page Speed on the contents of the input buffer, and put the results into
// the output buffer.
bool PageSpeedModule::RunPageSpeed(const NPVariant& document_arg,
                                   const NPVariant& filter_arg,
                                   NPVariant *result) {
  if (!NPVARIANT_IS_OBJECT(document_arg) && !NPVARIANT_IS_NULL(document_arg)) {
    return Throw("first argument to runPageSpeed must be an object or null");
  }
  if (!NPVARIANT_IS_STRING(filter_arg)) {
    return Throw("second argument to runPageSpeed must be a string");
  }

  output_start_ = 0;
  output_buffer_.clear();

  pagespeed::DomDocument* document = NULL;
  if (NPVARIANT_IS_OBJECT(document_arg)) {
    document = pagespeed_chromium::CreateDocument(
        npp_, NPVARIANT_TO_OBJECT(document_arg));
  }

  const NPString& filter_NPString = NPVARIANT_TO_STRING(filter_arg);
  const std::string filter_string(filter_NPString.UTF8Characters,
                                  filter_NPString.UTF8Length);

  const bool success = RunPageSpeedRules(NewFilter(filter_string), document,
                                         input_buffer_, &output_buffer_);
  if (!success) {
    return Throw("could not parse HAR");
  }

  input_buffer_.clear();
  if (result) {
    NULL_TO_NPVARIANT(*result);
  }
  return true;
}

// Produce either a string with the next chunk of data from the output buffer,
// or null to signify the end of the output.
bool PageSpeedModule::ReadMoreOutput(NPVariant *result) {
  if (result) {
    if (output_start_ >= output_buffer_.size()) {
      output_start_ = 0;
      output_buffer_.clear();
      NULL_TO_NPVARIANT(*result);
    } else {
      const size_t data_length =
          std::min(output_buffer_.size() - output_start_, kChunkSize);
      char* data_copy = static_cast<char*>(npnfuncs->memalloc(data_length));
      memcpy(data_copy, output_buffer_.data() + output_start_, data_length);
      STRINGN_TO_NPVARIANT(data_copy, data_length, *result);
      output_start_ += kChunkSize;
    }
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
  if (!strcmp(name, kAppendInputMethodId)) {
    has_method = true;
  } else if (!strcmp(name, kRunPageSpeedMethodId)) {
    has_method = true;
  } else if (!strcmp(name, kReadMoreOutputMethodId)) {
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
  if (!strcmp(name, kAppendInputMethodId)) {
    if (arg_count == 1) {
      rval = module->AppendInput(args[0], result);
    } else {
      rval = module->Throw("wrong number of arguments to appendInput");
    }
  } else if (!strcmp(name, kRunPageSpeedMethodId)) {
    if (arg_count == 2) {
      rval = module->RunPageSpeed(args[0], args[1], result);
    } else {
      rval = module->Throw("wrong number of arguments to runPageSpeed");
    }
  } else if (!strcmp(name, kReadMoreOutputMethodId)) {
    if (arg_count == 0) {
      rval = module->ReadMoreOutput(result);
    } else {
      rval = module->Throw("wrong number of arguments to readMoreOutput");
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

NPClass* GetNPSimpleClass() {
  return &kPageSpeedClass;
}
