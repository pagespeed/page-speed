// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PAGESPEED_CORE_STRING_UTIL_POSIX_H_
#define PAGESPEED_CORE_STRING_UTIL_POSIX_H_
#pragma once

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>

#include "base/logging.h"

namespace pagespeed {

namespace string_util {

// Chromium code style is to not use malloc'd strings; this is only for use
// for interaction with APIs that require it.
inline char* strdup(const char* str) {
  return ::strdup(str);
}

inline int strcasecmp(const char* string1, const char* string2) {
  return ::strcasecmp(string1, string2);
}

inline int strncasecmp(const char* string1, const char* string2, size_t count) {
  return ::strncasecmp(string1, string2, count);
}

inline int vsnprintf(char* buffer, size_t size,
                     const char* format, va_list arguments) {
  return ::vsnprintf(buffer, size, format, arguments);
}

}  // namespace string_util

}  // namespace pagespeed

#endif  // PAGESPEED_CORE_STRING_UTIL_POSIX_H_
