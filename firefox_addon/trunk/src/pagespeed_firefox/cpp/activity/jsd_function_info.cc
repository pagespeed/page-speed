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
// JsdFunctionInfo implementation. See jsd_function_info.h for
// details.

#include "jsd_function_info.h"

#include "nsDebug.h"
#include "nsStringAPI.h"
#include "nsXPCOM.h"

namespace activity {

JsdFunctionInfo::JsdFunctionInfo(jsdIScript *script)
    : script_(script),
      tag_(-1),
      file_name_(NULL),
      function_name_(NULL),
      function_source_utf8_(NULL) {
}

JsdFunctionInfo::~JsdFunctionInfo() {
  if (file_name_ != NULL) {
    NS_Free(file_name_);
  }
  if (function_name_ != NULL) {
    NS_Free(function_name_);
  }
  if (function_source_utf8_ != NULL) {
    NS_Free(function_source_utf8_);
  }
}

int32 JsdFunctionInfo::GetFunctionTag() {
  if (tag_ == -1) {
    PRUint32 tag = 0;
    nsresult rv = script_->GetTag(&tag);
    if (NS_FAILED(rv)) {
      NS_WARNING("Unable to get script tag");
      return -1;
    }

    tag_ = tag;
  }

  return tag_;
}

const char *JsdFunctionInfo::GetFileName() {
  if (file_name_ == NULL) {
    nsCString str;
    nsresult rv = script_->GetFileName(str);
    if (NS_FAILED(rv)) {
      NS_WARNING("Unable to get file name");
      return NULL;
    }
    file_name_ = NS_CStringCloneData(str);
  }
  return file_name_;
}

const char *JsdFunctionInfo::GetFunctionName() {
  if (function_name_ == NULL) {
    nsCString str;
    nsresult rv = script_->GetFunctionName(str);
    if (NS_FAILED(rv)) {
      NS_WARNING("Unable to get function name");
      return NULL;
    }
    function_name_ = NS_CStringCloneData(str);
  }
  return function_name_;
}

const char *JsdFunctionInfo::GetFunctionSourceUtf8() {
  if (function_source_utf8_ == NULL) {
    nsString function_source_str;

    // Modifies function_source_str
    nsresult rv = script_->GetFunctionSource(function_source_str);
    if (NS_FAILED(rv)) {
      NS_WARNING("Unable to get function source");
      return NULL;
    }

    // nsACString implementation that auto-converts from utf16 to utf8
    NS_ConvertUTF16toUTF8 function_source_utf8(function_source_str);

    // copy the internal data into a null-terminated buffer.
    function_source_utf8_ = NS_CStringCloneData(function_source_utf8);
  }

  return function_source_utf8_;
}

}  // namespace activity
