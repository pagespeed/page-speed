// Copyright 2010 Google Inc. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "pagespeed/core/string_util.h"

#include <errno.h>

#include <algorithm>  // for lexicographical_compare
#include <functional> // for trim
#include <cctype>     // for trim
#include <locale>     // for trim
#include <sstream>

namespace {

// Overloaded wrappers around vsnprintf. The buf_size parameter
// is the size of the buffer. These return the number of characters in the
// formatted string excluding the NUL terminator. If the buffer is not
// large enough to accommodate the formatted string without truncation, they
// return the number of characters that would be in the fully-formatted string
// (vsnprintf on Windows).
inline int vsnprintfT(char* buffer,
                      size_t buf_size,
                      const char* format,
                      va_list argptr) {
  return pagespeed::string_util::vsnprintf(buffer, buf_size, format, argptr);
}

bool CaseInsensitiveCompareChars(char x, char y) {
  return pagespeed::string_util::ToLowerASCII(x) <
      pagespeed::string_util::ToLowerASCII(y);
}

// trim from start
inline std::string& ltrim(std::string& s) {
  s.erase(s.begin(),
          std::find_if(s.begin(), s.end(),
                       std::not1(std::ptr_fun<char, bool>(
                           pagespeed::string_util::IsAsciiWhitespace))));
  return s;
}

// trim from end
inline std::string& rtrim(std::string& s) {
  s.erase(std::find_if(s.rbegin(), s.rend(),
                       std::not1(std::ptr_fun<char, bool>(
                           pagespeed::string_util::IsAsciiWhitespace))).base(),
          s.end());
  return s;
}

// trim from both ends
inline std::string& trim(std::string& s) {
  return ltrim(rtrim(s));
}

// Used by ReplaceStringPlaceholders to track the position in the string of
// replaced parameters.
struct ReplacementOffset {
  ReplacementOffset(uintptr_t parameter, size_t offset)
      : parameter(parameter),
        offset(offset) {}

  // Index of the parameter.
  uintptr_t parameter;

  // Starting position in the string.
  size_t offset;
};

static bool CompareParameter(const ReplacementOffset& elem1,
                             const ReplacementOffset& elem2) {
  return elem1.parameter < elem2.parameter;
}

}  // namespace

namespace pagespeed {

namespace string_util {

bool CaseInsensitiveStringComparator::operator()(const std::string& x,
                                                 const std::string& y) const {
  return std::lexicographical_compare(x.begin(), x.end(), y.begin(), y.end(),
                                      CaseInsensitiveCompareChars);
}

bool ContainsOnlyWhitespaceASCII(const std::string& str) {
  for (std::string::const_iterator i(str.begin()); i != str.end(); ++i) {
    if (!IsAsciiWhitespace(*i))
      return false;
  }
  return true;
}

bool StringCaseEqual(const base::StringPiece& s1,
                     const base::StringPiece& s2) {
  return (s1.size() == s2.size() &&
          0 == pagespeed::string_util::strncasecmp(s1.data(),
                                                   s2.data(),
                                                   s1.size()));
}

bool StringCaseStartsWith(const base::StringPiece& str,
                          const base::StringPiece& prefix) {
  return (str.size() >= prefix.size() &&
          0 == pagespeed::string_util::strncasecmp(str.data(),
                                                   prefix.data(),
                                                   prefix.size()));
}

bool StringCaseEndsWith(const base::StringPiece& str,
                        const base::StringPiece& suffix) {
  return (str.size() >= suffix.size() &&
          0 == pagespeed::string_util::strncasecmp(
                                  str.data() + str.size() - suffix.size(),
                                  suffix.data(), suffix.size()));
}

std::string IntToString(int value) {
  std::ostringstream sstream;
  sstream << value;
  return sstream.str();
}

std::string DoubleToString(double value) {
  std::ostringstream sstream;
  sstream << value;
  return sstream.str();
}

// Accepts a number string and returns its signed int representation.
// Returns false in case of overflow and sets output to INT_MAX.
// Returns false in case of underflow and sets output to INT_MIN.
// Returns false and sets output to zero
// in case the input string overflows long type.
bool StringToInt(const std::string& input, int* output) {

  std::string trimmed(input);
  trim(trimmed);
  if (trimmed.empty()) {
    *output = 0;
    return false;
  }

  // Points to the next char in the input string
  // once strtol invocation completes.
  char* next_char;

  // Reset errno before invoking strtol, as some implementations of
  // strtol do not clear the value on success.
  errno = 0;

  // strtol(input_str, next_char, base): converts the given input string
  // to long number and returns once the conversion is done.
  // next_char pointer points to the next character to
  // where the conversion is finalized.
  long int result = strtol(trimmed.c_str(), &next_char, 10);

  // strtol will consume characters until it encounters either end of
  // string or an invalid (non-numeric) character.
  const bool found_invalid_char = *next_char != '\0';

  // when the input is outside of the valid range for 'long int',
  // strtol returns ERANGE and sets the output value to either
  // LONG_MIN or LONG_MAX accordingly. note that LONG_MIN and LONG_MAX
  // may be the same or differ from INT_MIN and INT_MAX, depending on
  // architecture.
  bool overflowed = errno == ERANGE;

  // Clamp to [INT_MIN, INT_MAX] in cases where sizeof(long int) >
  // sizeof(int), e.g. x86_64.
  if (result > INT_MAX) {
    result = INT_MAX;
    overflowed = true;
  } else if (result < INT_MIN) {
    result = INT_MIN;
    overflowed = true;
  }

  *output = static_cast<int>(result);

  return !found_invalid_char && !overflowed;
}

std::string JoinString(const std::vector<std::string>& parts, char sep) {
  if (parts.empty())
      return std::string();

  std::string result(parts[0]);
  std::vector<std::string>::const_iterator iter = parts.begin();
  ++iter;

  for (; iter != parts.end(); ++iter) {
    result += sep;
    result += *iter;
  }

  return result;
}


std::string ReplaceStringPlaceholders(
  const base::StringPiece& format_string,
  const std::vector<std::string>& subst,
  std::vector<size_t>* offsets) {

 size_t substitutions = subst.size();

 size_t sub_length = 0;
 for (std::vector<std::string>::const_iterator iter = subst.begin();
      iter != subst.end(); ++iter) {
   sub_length += iter->length();
 }

 std::string formatted;
 formatted.reserve(format_string.length() + sub_length);

 std::vector<ReplacementOffset> r_offsets;
 for (base::StringPiece::const_iterator i = format_string.begin();
      i != format_string.end(); ++i) {
   if ('$' == *i) {
     if (i + 1 != format_string.end()) {
       ++i;
       DCHECK('$' == *i || '1' <= *i) << "Invalid placeholder: " << *i;
       if ('$' == *i) {
         while (i != format_string.end() && '$' == *i) {
           formatted.push_back('$');
           ++i;
         }
         --i;
       } else {
         uintptr_t index = 0;
         while (i != format_string.end() && '0' <= *i && *i <= '9') {
           index *= 10;
           index += *i - '0';
           ++i;
         }
         --i;
         index -= 1;
         if (offsets) {
           ReplacementOffset r_offset(index,
               static_cast<int>(formatted.size()));
           r_offsets.insert(std::lower_bound(r_offsets.begin(),
                                             r_offsets.end(),
                                             r_offset,
                                             &CompareParameter),
                            r_offset);
         }
         if (index < substitutions)
           formatted.append(subst.at(index));
       }
     }
   } else {
     formatted.push_back(*i);
   }
 }
 if (offsets) {
   for (std::vector<ReplacementOffset>::const_iterator i = r_offsets.begin();
        i != r_offsets.end(); ++i) {
     offsets->push_back(i->offset);
   }
 }
 return formatted;
}

void StringAppendV(std::string* dst, const char* format, va_list ap) {
  // First try with a small fixed size buffer.
  // This buffer size should be kept in sync with StringUtilTest.GrowBoundary
  // and StringUtilTest.StringPrintfBounds.
  std::string::value_type stack_buf[1024];

  va_list ap_copy;
#if defined(_WIN32)
  ap_copy = ap;
#else
  va_copy(ap_copy, ap);
#endif

#if !defined(_WIN32)
  errno = 0;
#endif
  int result = vsnprintfT(stack_buf, arraysize(stack_buf), format, ap_copy);
  va_end(ap_copy);

  if (result >= 0 && result < static_cast<int>(arraysize(stack_buf))) {
    // It fit.
    dst->append(stack_buf, result);
    return;
  }

  // Repeatedly increase buffer size until it fits.
  int mem_length = arraysize(stack_buf);
  while (true) {
    if (result < 0) {
#if !defined(_WIN32)
      // On Windows, vsnprintfT always returns the number of characters in a
      // fully-formatted string, so if we reach this point, something else is
      // wrong and no amount of buffer-doubling is going to fix it.
      if (errno != 0 && errno != EOVERFLOW)
#endif
      {
        // If an error other than overflow occurred, it's never going to work.
        DLOG(WARNING) << "Unable to printf the requested string due to error.";
        return;
      }
      // Try doubling the buffer size.
      mem_length *= 2;
    } else {
      // We need exactly "result + 1" characters.
      mem_length = result + 1;
    }

    if (mem_length > 32 * 1024 * 1024) {
      // That should be plenty, don't try anything larger.  This protects
      // against huge allocations when using vsnprintfT implementations that
      // return -1 for reasons other than overflow without setting errno.
      DLOG(WARNING) << "Unable to printf the requested string due to size.";
      return;
    }

    std::vector<std::string::value_type> mem_buf(mem_length);

    // NOTE: You can only use a va_list once.  Since we're in a while loop, we
    // need to make a new copy each time so we don't use up the original.
#if defined(_WIN32)
    ap_copy = ap;
#else
    va_copy(ap_copy, ap);
#endif
    result = vsnprintfT(&mem_buf[0], mem_length, format, ap_copy);
    va_end(ap_copy);

    if ((result >= 0) && (result < mem_length)) {
      // It fit.
      dst->append(&mem_buf[0], result);
      return;
    }
  }
}

std::string StringPrintf(const char* format, ...) {
 va_list ap;
 va_start(ap, format);
 std::string result;
 StringAppendV(&result, format, ap);
 va_end(ap);
 return result;
}

bool LowerCaseEqualsASCII(const std::string& a, const char* b) {
  for (std::string::const_iterator it = a.begin(); it != a.end(); ++it, ++b) {
      if (!*b || ToLowerASCII(*it) != *b)
        return false;
    }
    return *b == 0;
}

void TrimWhitespaceASCII(const std::string& input,
                         TrimPositions positions,
                         std::string* output) {
  output->assign(input);
  if ((positions & TRIM_LEADING) != 0) {
    ltrim(*output);
  }
  if ((positions & TRIM_TRAILING) != 0) {
    rtrim(*output);
  }
}

}  // namespace string_util

}  // namespace pagespeed
