// Copyright 2010 The Native Client SDK Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#if defined (__native_client__)
#include <nacl/npupp.h>
#else
#include "third_party/npapi/bindings/npapi.h"
#include "third_party/npapi/bindings/nphostapi.h"
#endif

// These functions are called when a module instance is first loaded, and when
// the module instance is finally deleted.  They must use C-style linkage.

extern "C" {
// Populates |plugin_funcs| by calling InitializePluginFunctions.
// Declaration: npupp.h
// Web Reference: N/A
NPError NP_GetEntryPoints(NPPluginFuncs* plugin_funcs) {
  extern NPError InitializePluginFunctions(NPPluginFuncs* plugin_funcs);
  return InitializePluginFunctions(plugin_funcs);
}


// Some platforms, including Native Client, use the two-parameter version of
// NP_Initialize(), and do not call NP_GetEntryPoints().  Others (Mac, e.g.)
// use single-parameter version of NP_Initialize(), and then call
// NP_GetEntryPoints() to get the NPP functions.  Also, the NPN entry points
// are defined by the Native Client loader, but are not defined in the trusted
// plugin loader (and must be filled in in NP_Initialize()).

// Called when the first instance of this plugin is first allocated to
// initialize global state.  The browser is hereby telling the plugin its
// interface in |browser_functions| and expects the plugin to populate
// |plugin_functions| in return.  Memory allocated by this function may only
// be cleaned up by NP_Shutdown.
// returns an NPError if anything went wrong.
// Declaration: npupp.h
// Documentation URL: https://developer.mozilla.org/en/NP_Initialize
NPError NP_Initialize(NPNetscapeFuncs* browser_functions,
                      NPPluginFuncs* plugin_functions) {
  return NP_GetEntryPoints(plugin_functions);
}

// Called just before the plugin itself is completely unloaded from the
// browser.  Should clean up anything allocated by NP_Initialize.
// Declaration: npupp.h
// Documentation URL: https://developer.mozilla.org/en/NP_Shutdown
NPError NP_Shutdown() {
  return NPERR_NO_ERROR;
}

}  // extern "C"
