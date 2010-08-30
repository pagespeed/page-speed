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

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <algorithm>  // for std::min()
#include <sstream>
#include <string>
#include <vector>

#ifdef __native_client__
 #include <nacl/nacl_npapi.h>
#else  // Building a develop version for debugging.
 #include "third_party/npapi/bindings/npapi.h"
 #include "third_party/npapi/bindings/nphostapi.h"
#endif

#include "base/logging.h"
#include "base/scoped_ptr.h"
#include "base/stl_util-inl.h"
#include "pagespeed/core/engine.h"
#include "pagespeed/core/formatter.h"
#include "pagespeed/core/pagespeed_input.h"
#include "pagespeed/core/resource_filter.h"
#include "pagespeed/core/rule.h"
#include "pagespeed/filters/ad_filter.h"
#include "pagespeed/filters/tracker_filter.h"
#include "pagespeed/formatters/json_formatter.h"
#include "pagespeed/har/http_archive.h"
#include "pagespeed/rules/rule_provider.h"

namespace {

// These are the method names as JavaScript sees them:
const char* kAppendInputMethodId = "appendInput";
const char* kRunPageSpeedMethodId = "runPageSpeed";
const char* kReadMoreOutputMethodId = "readMoreOutput";

// Buffers for input/output; data has to be transferred a piece at a time,
// because SRPC currently can't handle strings larger than one or two dozen
// kilobytes.
std::string input_buffer;
std::string output_buffer;
size_t output_start = 0;
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
bool RunPageSpeedRules(pagespeed::ResourceFilter* filter,
                       const std::string& har_data,
                       std::string* output) {
  scoped_ptr<pagespeed::PagespeedInput> input(
      pagespeed::ParseHttpArchiveWithFilter(har_data, filter));

  if (input.get() == NULL) {
    return false;
  }

  input->Freeze();

  std::vector<pagespeed::Rule*> rules;

  // In environments where exceptions can be thrown, use
  // STLElementDeleter to make sure we free the rules in the event
  // that they are not transferred to the Engine.
  STLElementDeleter<std::vector<pagespeed::Rule*> > rule_deleter(&rules);

  const bool save_optimized_content = false;
  pagespeed::rule_provider::AppendAllRules(save_optimized_content, &rules);

  // Ownership of rules is transferred to the Engine instance.
  pagespeed::Engine engine(&rules);
  engine.Init();

  std::stringstream stream;
  pagespeed::formatters::JsonFormatter formatter(&stream, NULL);

  engine.ComputeAndFormatResults(*input, &formatter);

  output->append(stream.str());

  return true;
}

// Accept a chunk of data as a string, and append it to the input buffer.
bool AppendInput(const NPVariant& argument, NPVariant *result) {
  if (result && NPVARIANT_IS_STRING(argument)) {
    const NPString& arg_NPString = NPVARIANT_TO_STRING(argument);
    input_buffer.append(arg_NPString.UTF8Characters,
                        arg_NPString.UTF8Length);
    NULL_TO_NPVARIANT(*result);
  }
  return true;
}

// Run Page Speed on the contents of the input buffer, and put the results into
// the output buffer.
bool RunPageSpeed(const NPVariant& argument, NPVariant *result) {
  if (result && NPVARIANT_IS_STRING(argument)) {
    output_start = 0;
    output_buffer.clear();

    const NPString& arg_NPString = NPVARIANT_TO_STRING(argument);
    const std::string arg_string(arg_NPString.UTF8Characters,
                                 arg_NPString.UTF8Length);

    const bool success = RunPageSpeedRules(NewFilter(arg_string),
                                           input_buffer, &output_buffer);
    if (!success) {
      output_buffer.append("Error reading HAR");
    }
    input_buffer.clear();
    NULL_TO_NPVARIANT(*result);
  }
  return true;
}

// Produce either a string with the next chunk of data from the output buffer,
// or null to signify the end of the output.
bool ReadMoreOutput(NPVariant *result) {
  if (result) {
    if (output_start >= output_buffer.size()) {
      output_start = 0;
      output_buffer.clear();
      NULL_TO_NPVARIANT(*result);
    } else {
      const size_t data_length = std::min(output_buffer.size() - output_start,
                                          kChunkSize);
      char* data_copy = reinterpret_cast<char*>(NPN_MemAlloc(data_length));
      memcpy(data_copy, output_buffer.data() + output_start, data_length);
      STRINGN_TO_NPVARIANT(data_copy, data_length, *result);
      output_start += kChunkSize;
    }
  }
  return true;
}

NPObject* Allocate(NPP npp, NPClass* npclass) {
  return new NPObject;
}

void Deallocate(NPObject* object) {
  delete object;
}

// Return true if method_name is a recognized method.
bool HasMethod(NPObject* obj, NPIdentifier method_name) {
  char *name = NPN_UTF8FromIdentifier(method_name);
  bool has_method = false;
  if (!strcmp((const char *)name, kAppendInputMethodId)) {
    has_method = true;
  } else if (!strcmp((const char *)name, kRunPageSpeedMethodId)) {
    has_method = true;
  } else if (!strcmp((const char *)name, kReadMoreOutputMethodId)) {
    has_method = true;
  }
  NPN_MemFree(name);
  return has_method;
}

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
  char *name = NPN_UTF8FromIdentifier(method_name);
  if (name == NULL) {
    return false;
  }
  bool rval = false;
  // Map the method name to a function call.  |result| is filled in by the
  // called function, then gets returned to the browser when Invoke() returns.
  if (!strcmp((const char *)name, kAppendInputMethodId)) {
    if (arg_count >= 1) {
      rval = AppendInput(args[0], result);
    }
  } else if (!strcmp((const char *)name, kRunPageSpeedMethodId)) {
    if (arg_count >= 1) {
      rval = RunPageSpeed(args[0], result);
    }
  } else if (!strcmp((const char *)name, kReadMoreOutputMethodId)) {
    rval = ReadMoreOutput(result);
  }
  // Since name was allocated above by NPN_UTF8FromIdentifier,
  // it needs to be freed here.
  NPN_MemFree(name);
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

NPClass *GetNPSimpleClass() {
  return &kPageSpeedClass;
}
