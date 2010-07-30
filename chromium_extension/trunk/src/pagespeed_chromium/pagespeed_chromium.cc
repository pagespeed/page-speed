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

#include <string>
#include <vector>

#ifdef __native_client__
 #include <nacl/nacl_npapi.h>
#else  // Building a develop version for debugging.
 #include "third_party/npapi/bindings/npapi.h"
 #include "third_party/npapi/bindings/nphostapi.h"
#endif

namespace {

// This is the method name as JavaScript sees it:
const char* kRunPageSpeedMethodId = "runPageSpeed";

// This function creates a string in the browser's memory pool and then returns
// a variable containing a pointer to that string.  The variable is later
// returned back to the browser by the Invoke() function that called this.
bool RunPageSpeed(const NPVariant& argument, NPVariant *result) {
  bool ok = true;
  if (result && NPVARIANT_IS_STRING(argument)) {
    // Get the argument:
    const NPString& arg_NPString = NPVARIANT_TO_STRING(argument);
    const std::string arg_string(arg_NPString.UTF8Characters,
                                 arg_NPString.UTF8Length);

    // Produce the result:
    // TODO(mdsteele): Make this actually run the Page Speed library.
    std::string output("Input was: ");
    output.append(arg_string);

    // Return the result:
    const char *msg = output.c_str();
    const int msg_length = strlen(msg) + 1;
    // Note: |msg_copy| will be freed later on by the browser, so it needs to
    // be allocated here with NPN_MemAlloc().
    char *msg_copy = reinterpret_cast<char*>(NPN_MemAlloc(msg_length));
    strncpy(msg_copy, msg, msg_length);
    STRINGN_TO_NPVARIANT(msg_copy, msg_length - 1, *result);
  }
  return ok;
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
  if (!strcmp((const char *)name, kRunPageSpeedMethodId)) {
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
  if (!strcmp((const char *)name, kRunPageSpeedMethodId)) {
    if (arg_count >= 1) {
      rval = RunPageSpeed(args[0], result);
    }
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
