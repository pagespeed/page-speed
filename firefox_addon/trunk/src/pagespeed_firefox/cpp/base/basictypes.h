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

#ifndef BASE_BASICTYPES_H_
#define BASE_BASICTYPES_H_

typedef signed char         schar;
typedef signed char         int8;
typedef short               int16;
// TODO(mbelshe) Remove these type guards.  These are
//               temporary to avoid conflicts with npapi.h.
#ifndef _INT32
#define _INT32
typedef int                 int32;
#endif

// The NSPR system headers define 64-bit as |long| when possible.  In order to
// not have typedef mismatches, we do the same on LP64.
#if __LP64__
typedef long                int64;
#else
typedef long long           int64;
#endif

// A macro to disallow the copy constructor and operator= functions
// This should be used in the private: declarations for a class
#define DISALLOW_COPY_AND_ASSIGN(TypeName) \
  TypeName(const TypeName&);               \
  void operator=(const TypeName&)

#endif  // BASE_BASICTYPES_H_
