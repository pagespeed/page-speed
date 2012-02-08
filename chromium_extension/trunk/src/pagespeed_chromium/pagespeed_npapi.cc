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

#include "pagespeed_chromium/pagespeed_npapi.h"

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
#include "base/string_number_conversions.h"
#include "base/string_util.h"
#include "pagespeed/core/pagespeed_init.h"
#include "pagespeed_chromium/pagespeed_chromium.h"

namespace {

// These are the method names as JavaScript sees them:
const char* kPingMethodId = "ping";
const char* kRunPageSpeedMethodId = "runPageSpeed";

// NPAPI doesn't need ID tracking, so we just use a dummy value.
const char* kDummyId = "";

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
                                   NPVariant *result) {
  // Instantiate an AtExitManager so our Singleton<>s are able to
  // schedule themselves for destruction.
  base::AtExitManager at_exit_manager;

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

  std::string output, error_string;
  // RunPageSpeedRules will deallocate the filter and the document.
  const bool success = pagespeed_chromium::RunPageSpeedRules(
      kDummyId, har_string, document_string, timeline_string, filter_string,
      locale_string, save_optimized_content, &output, &error_string);
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
    if (arg_count == 6) {
      rval = module->RunPageSpeed(args[0], args[1], args[2], args[3], args[4],
                                  args[5], result);
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
