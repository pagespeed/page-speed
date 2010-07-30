// Copyright 2010 The Native Client SDK Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#include <assert.h>
#if defined (__native_client__)
#include <nacl/npapi_extensions.h>
#include <nacl/npupp.h>
#else
#include "third_party/npapi/bindings/npapi.h"
#include "third_party/npapi/bindings/npapi_extensions.h"
#include "third_party/npapi/bindings/nphostapi.h"
#endif

#if !defined(__native_client__)
// The development version needs to call through to the browser directly.  These
// wrapper routines are not required when making the published version.

static NPNetscapeFuncs kBrowserFuncs = { 0 };

extern "C" {

void InitializeBrowserFunctions(NPNetscapeFuncs* browser_functions) {
  memcpy(&kBrowserFuncs, browser_functions, sizeof(kBrowserFuncs));
}

}  // extern "C"

NPUTF8* NPN_UTF8FromIdentifier(NPIdentifier identifier) {
  return kBrowserFuncs.utf8fromidentifier(identifier);
}

void* NPN_MemAlloc(uint32 size) {
  return kBrowserFuncs.memalloc(size);
}

void NPN_MemFree(void* mem) {
  kBrowserFuncs.memfree(mem);
}

NPObject* NPN_CreateObject(NPP npp, NPClass* np_class) {
  return kBrowserFuncs.createobject(npp, np_class);
}

NPObject* NPN_RetainObject(NPObject* obj) {
  return kBrowserFuncs.retainobject(obj);
}

NPError NPN_GetValue(NPP instance, NPNVariable variable, void* value) {
  return kBrowserFuncs.getvalue(instance, variable, value);
}

#endif
