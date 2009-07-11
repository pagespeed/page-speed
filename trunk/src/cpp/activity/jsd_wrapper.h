/**
 * Copyright 2008-2009 Google Inc.
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

// Author: Bryan McQuade

// Wrapper around the jsdIDebuggerService. jsdIDebuggerService.idl
// includes nsAString.h, even though it only needs to forward-declare
// nsAString. Unfortunately, nsAString.h is a mozilla-internal API, so
// when we attempt to include jsdIDebuggerService.h, our builds
// fail. To remedy this, we forward-declare the structures we depend
// on, and #define the include guard for nsAString.h, which prevents
// nsAString.h from failing our build.

#ifndef JSD_WRAPPER_H_
#define JSD_WRAPPER_H_

// Use the jsdIDebuggerService.h from Firefox 3.0.11
#include "jsdIDebuggerService_3_0_11.h"

// For now, map the names of various jsd classes to the classes
// associated with version 3.0.11.
typedef jsdICallHook_3_0_11 jsdICallHook;
typedef jsdIDebuggerService_3_0_11 jsdIDebuggerService;
typedef jsdIScript_3_0_11 jsdIScript;
typedef jsdIScriptHook_3_0_11 jsdIScriptHook;
typedef jsdIStackFrame_3_0_11 jsdIStackFrame;
#define NS_DECL_JSDICALLHOOK NS_DECL_JSDICALLHOOK_3_0_11
#define NS_DECL_JSDISCRIPTHOOK NS_DECL_JSDISCRIPTHOOK_3_0_11

#endif  // JSD_WRAPPER_H_
