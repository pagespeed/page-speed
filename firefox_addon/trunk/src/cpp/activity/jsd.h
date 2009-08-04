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

// Author: Bryan McQuade

// Include the default version if the jsdIDebuggerService and defines
// the necessary typedefs for the subset of the jsdIDebuggerService
// API that we need.

#ifndef JSD_H_
#define JSD_H_

#include "jsdIDebuggerService_3_5.h"

// Map the names of various jsd classes to the classes
// associated with version 3.5.
typedef jsdICallHook_3_5 jsdICallHook;
typedef jsdIDebuggerService_3_5 jsdIDebuggerService;
typedef jsdIScript_3_5 jsdIScript;
typedef jsdIScriptHook_3_5 jsdIScriptHook;
typedef jsdIStackFrame_3_5 jsdIStackFrame;
#define NS_DECL_JSDICALLHOOK NS_DECL_JSDICALLHOOK_3_5
#define NS_DECL_JSDISCRIPTHOOK NS_DECL_JSDISCRIPTHOOK_3_5

#endif  // JSD_H_
