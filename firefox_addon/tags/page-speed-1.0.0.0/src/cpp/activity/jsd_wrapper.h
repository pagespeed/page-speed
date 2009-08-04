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

// Forward declare the classes that jsdIDebuggerService cares about.
class nsAString;
class JSContext;
class JSDContext;
class JSRuntime;
class JSDStackFrameInfo;
class JSDThreadState;
class JSDProperty;
class JSDScript;
class JSDValue;
class JSDObject;

#ifndef nsAString_h___
// Temporarily define the nsAString.h include guard, to prevent it
// from failing the build.
#define nsAString_h___
#define NS_ASTRING_NOT_INCLUDED 1
#endif

#include "jsdIDebuggerService.h"

// Undefine the nsAString.h include guard if it wasn't already
// defined.
#if NS_ASTRING_NOT_INCLUDED
#undef nsAString_h___
#endif

#endif  // JSD_WRAPPER_H_
