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
//
// Platform-specific includes and namespaces for hash_map.

#ifndef PORTABLE_HASH_MAP_H_
#define PORTABLE_HASH_MAP_H_

#if defined(__GNUC__)

#include <ext/hash_map>

template<typename KEY, typename VALUE>
class portable_hash_map : public __gnu_cxx::hash_map<KEY, VALUE> {};

#elif defined(_WINDOWS)

#include <hash_map>

template<typename KEY, typename VALUE>
class portable_hash_map : public stdext::hash_map<KEY, VALUE> {};

#else

#error Please update portable_hash_map.h to support your platform.

#endif

#endif  // PORTABLE_HASH_MAP_H_
