// Copyright 2007, Google Inc.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

// ICU integration functions.

#include <stdlib.h>
#include <string.h>

#include "googleurl/src/url_canon_icu.h"
#include "googleurl/src/url_canon_internal.h"  // for _itoa_s

#include "base/logging.h"
#include "base/third_party/icu/icu_utf.h"

namespace url_canon {

ICUCharsetConverter::ICUCharsetConverter(UConverter* converter)
    : converter_(converter) {
}

void ICUCharsetConverter::ConvertFromUTF16(const char16* input,
                                           int input_len,
                                           CanonOutput* output) {
  LOG(INFO) << "ConvertFromUTF16 not supported (non-icu build)";
}

bool IDNToASCII(const char16* src, int src_len, CanonOutputW* output) {
  LOG(INFO) << "IDNToASCII not supported (non-icu build)";
  return false;
}

bool ReadUTFChar(const char* str, int* begin, int length,
                 unsigned* code_point_out) {
  int code_point;  // Avoids warning when U8_NEXT writes -1 to it.
  CBU8_NEXT(str, *begin, length, code_point);
  *code_point_out = static_cast<unsigned>(code_point);

  // The ICU macro above moves to the next char, we want to point to the last
  // char consumed.
  (*begin)--;

  // Validate the decoded value.
  if (CBU_IS_UNICODE_CHAR(code_point))
    return true;
  *code_point_out = kUnicodeReplacementCharacter;
  return false;
}

bool ReadUTFChar(const char16* str, int* begin, int length,
                 unsigned* code_point) {
  if (CBU16_IS_SURROGATE(str[*begin])) {
    if (!CBU16_IS_SURROGATE_LEAD(str[*begin]) || *begin + 1 >= length ||
        !CBU16_IS_TRAIL(str[*begin + 1])) {
      // Invalid surrogate pair.
      *code_point = kUnicodeReplacementCharacter;
      return false;
    } else {
      // Valid surrogate pair.
      *code_point = CBU16_GET_SUPPLEMENTARY(str[*begin], str[*begin + 1]);
      (*begin)++;
    }
  } else {
    // Not a surrogate, just one 16-bit word.
    *code_point = str[*begin];
  }

  if (CBU_IS_UNICODE_CHAR(*code_point))
    return true;

  // Invalid code point.
  *code_point = kUnicodeReplacementCharacter;
  return false;
}

}  // namespace url_canon
