// Copyright 2010 Google Inc. All Rights Reserved.
// Author: bmcquade@google.com (Bryan McQuade)

#ifndef NET_INSTAWEB_HTMLPARSE_PUBLIC_PRINTF_FORMAT_H_
#define NET_INSTAWEB_HTMLPARSE_PUBLIC_PRINTF_FORMAT_H_

// From Chromium svn (base/compiler_specific.h)

#if defined(COMPILER_GCC)

// Tell the compiler a function is using a printf-style format string.
// |format_param| is the one-based index of the format string parameter;
// |dots_param| is the one-based index of the "..." parameter.
// For v*printf functions (which take a va_list), pass 0 for dots_param.
// (This is undocumented but matches what the system C headers do.)
#define INSTAWEB_PRINTF_FORMAT(format_param, dots_param) \
    __attribute__((format(printf, format_param, dots_param)))

#else  // Not GCC

#define INSTAWEB_PRINTF_FORMAT(x, y)

#endif

#endif  // NET_INSTAWEB_HTMLPARSE_PUBLIC_PRINTF_FORMAT_H_
