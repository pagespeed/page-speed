// Copyright 2010 The Native Client SDK Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#if defined (__native_client__)
#include <nacl/npupp.h>
#else
#include "third_party/npapi/bindings/npapi.h"
#include "third_party/npapi/bindings/nphostapi.h"
#endif

// These functions are required by both the develop and publish versions,
// they are called when a module instance is first loaded, and when the module
// instance is finally deleted.  They must use C-style linkage.

extern "C" {

NPError NP_GetEntryPoints(NPPluginFuncs* plugin_funcs) {
  extern NPError InitializePluginFunctions(NPPluginFuncs* plugin_funcs);
  return InitializePluginFunctions(plugin_funcs);
}

// Some platforms, including Native Client uses the two-parameter version of
// NP_Initialize(), and do not call NP_GetEntryPoints().  Others (Mac, e.g.)
// use single-parameter version of NP_Initialize(), and then call
// NP_GetEntryPoints() to get the NPP functions.  Also, the NPN entry points are
// defined by the Native Client loader, but are not defined in the trusted
// plugin loader (and must be filled in in NP_Initialize()).
#if defined(__native_client__)
NPError NP_Initialize(NPNetscapeFuncs* browser_functions,
                      NPPluginFuncs* plugin_functions) {
  return NP_GetEntryPoints(plugin_functions);
}
#elif defined(OS_LINUX)
NPError NP_Initialize(NPNetscapeFuncs* browser_functions,
                      NPPluginFuncs* plugin_functions) {
  extern void InitializeBrowserFunctions(NPNetscapeFuncs* browser_functions);
  InitializeBrowserFunctions(browser_functions);
  return NP_GetEntryPoints(plugin_functions);
}
#elif defined(OS_MACOSX)
NPError NP_Initialize(NPNetscapeFuncs* browser_functions) {
  extern void InitializeBrowserFunctions(NPNetscapeFuncs* browser_functions);
  InitializeBrowserFunctions(browser_functions);
  return NPERR_NO_ERROR;
}
#elif defined(OS_WIN)
NPError NP_Initialize(NPNetscapeFuncs* browser_functions) {
  extern void InitializeBrowserFunctions(NPNetscapeFuncs* browser_functions);
  InitializeBrowserFunctions(browser_functions);
  return NPERR_NO_ERROR;
}
#else
#error "Unrecognized platform"
#endif

NPError NP_Shutdown() {
  return NPERR_NO_ERROR;
}

#if !defined(__native_client__) && defined(OS_LINUX)
NPError NP_GetValue(NPP instance, NPPVariable variable, void* value) {
  extern NPError NPP_GetValue(NPP instance, NPPVariable variable, void* value);
  NPError err = NPERR_NO_ERROR;
  switch (variable) {
    case NPPVpluginNameString:
      *(static_cast<const char**>(value)) = "Page Speed";
      break;
    case NPPVpluginDescriptionString:
      *(static_cast<const char**>(value)) = "Google Page Speed";
      break;
    case NPPVpluginNeedsXEmbed:
      *(static_cast<NPBool*>(value)) = TRUE;
      break;
    default:
      err = NPP_GetValue(instance, variable, value);
      break;
  }
  return err;
}

// Note that this MIME type has to match the type in the <embed> tag used to
// load the develop version of the module.  See the Mozilla docs for more info
// on the MIME type format:
//   https://developer.mozilla.org/En/NP_GetMIMEDescription
const char* NP_GetMIMEDescription(void) {
  return "pepper-application/pagespeed:nexe:Page Speed";
}
#endif

}  // extern "C"
